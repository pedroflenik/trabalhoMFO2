#include <assert.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "nlohmann/json.hpp"
#include <string>
#include <unordered_map>

#include "bank.hpp"

using json = nlohmann::json;
using namespace std;

enum class Action {
  Init,
  Deposit,
  Withdraw,
  Transfer,
  BuyInvestment,
  SellInvestment,
  Unknown
};

void mostraBanco(const BankState& bank_state) {
    cout << "Balances:" << endl;
    for (const auto& balance : bank_state.balances) {
        cout << balance.first << ": " << balance.second << endl;
    }

    cout << "\nInvestments:" << endl;
    for (const auto& investment : bank_state.investments) {
        cout << "Investment ID: " << investment.first << endl;
        cout << "  Owner: " << investment.second.owner << endl;
        cout << "  Amount: " << investment.second.amount << endl;
    }

    cout << "\nNext ID: " << bank_state.next_id << endl;
}

bool compareBankState(const BankState& bs1, const BankState& bs2) {
    if (bs1.balances != bs2.balances) {
        return false;
    }

    if (bs1.investments != bs2.investments) {
        return false;
    }
    if (bs1.next_id != bs2.next_id) {
        return false;
    }

    return true;
}

Action stringToAction(const std::string &actionStr) {
  static const std::unordered_map<std::string, Action> actionMap = {
      {"q::init", Action::Init},
      {"deposit_action", Action::Deposit},
      {"withdraw_action", Action::Withdraw},
      {"transfer_action", Action::Transfer},
      {"buy_investment_action", Action::BuyInvestment},
      {"sell_investment_action", Action::SellInvestment}};

  auto it = actionMap.find(actionStr);
  if (it != actionMap.end()) {
    return it->second;
  } else {
    return Action::Unknown;
  }
}

int int_from_json(json j) {
  string s = j["#bigint"];
  return stoi(s);
}

map<string, int> balances_from_json(json j) {
  map<string, int> m;
  for (auto it : j["#map"]) {
    m[it[0]] = int_from_json(it[1]);
  }
  return m;
}

map<int, Investment> investments_from_json(json j) {
  map<int, Investment> m;
  for (auto it : j["#map"]) {
    m[int_from_json(it[0])] = {.owner = it[1]["owner"],
                               .amount = int_from_json(it[1]["amount"])};
  }
  return m;
}

BankState bank_state_from_json(json state) {
  map<string, int> balances = balances_from_json(state["balances"]);
  map<int, Investment> investments =
      investments_from_json(state["investments"]);
  int next_id = int_from_json(state["next_id"]);
  return {.balances = balances, .investments = investments, .next_id = next_id};
}

int main() {

  int sucessCount{0};
  int errorCount{0};
  string acao = "";
  for (int i = 0; i < 10000; i++) {
    cout << "Trace #" << i << endl;
    std::ifstream f("traces/out" + to_string(i) + ".itf.json");
    json data = json::parse(f);

    BankState bank_state = bank_state_from_json(data["states"][0]["bank_state"]);
    auto states = data["states"];
    for (auto state : states) {

      string action = state["action_taken"];
      json nondet_picks = state["nondet_picks"];

      string error = "";
      //displayBankState(bank_state);
      switch (stringToAction(action)) {
        case Action::Init: {
          cout << "initializing" << endl;
          break;
        }
        case Action::Deposit: { 
          acao =  "Deposit";
          string depositor = nondet_picks["depositor"]["value"];
          int amount = int_from_json(nondet_picks["amount"]["value"]);
          error = deposit(bank_state,depositor,amount);
          
          break;
        }
        case Action::Withdraw:{
           acao =   "Withdraw";
            string withdrawer = nondet_picks["withdrawer"]["value"];
            int amount = int_from_json(nondet_picks["amount"]["value"]);
            error = withdraw(bank_state,withdrawer,amount);

            break;
        }
        case Action::Transfer: {
          acao =  "Transfer";
          string sender = nondet_picks["sender"]["value"];
          string receiver = nondet_picks["receiver"]["value"];
          int amount = int_from_json(nondet_picks["amount"]["value"]);
          error = transfer(bank_state,sender,receiver,amount);
         
          break;
        }
        case Action::BuyInvestment:{
          acao =  "BuyInvestment";
          string buyer = nondet_picks["buyer"]["value"];
          int amount = int_from_json(nondet_picks["amount"]["value"]);
          error = buy_investment(bank_state,buyer,amount);
          break;
        }
        case Action::SellInvestment:{
          acao =  "SellInvestment";
          string seller = nondet_picks["seller"]["value"];
          int id = int_from_json(nondet_picks["id"]["value"]);
          error = sell_investment(bank_state,seller,id);
          break;
        }
        default: {
          cout << "TODO: fazer a conexão para as outras ações. Ação: " << action << endl;
          error = "";
          break;
        }
      }
      BankState expected_bank_state = bank_state_from_json(state["bank_state"]);
      string expected_error = string(state["error"]["tag"]).compare("Some") == 0
                                  ? state["error"]["value"]
                                  : "";


      cout << "ACAO: " << acao << endl;
      if (compareBankState(bank_state,expected_bank_state)) {
          std::cout << "SUCESSO\n";
          sucessCount++;
      }
      if(error == expected_error){
        std::cout << "Erro obtido == erro esperado" << endl;
      }else{
        errorCount++;
        std::cout << "Erro obtido != erro esperado" << endl;
        std::cout<< "ERRO: " << error << endl;
        std::cout << "Erro esperado: " << expected_error << endl << endl;
        std::cout << "Estado do banco: \n";
        //mostraBanco(bank_state); 
        return 1; // Tire o return para rodar todos os testes
      }
      
      

      
      //cout << "TODO: comparar o estado esperado com o estado obtido" << endl;
      //cout << "TODO: comparar o erro esperado com o erro obtido" << endl;
    }
  
     
  }

  cout << "TESTES SUCESSO: " << sucessCount << endl;
  cout << "TESTES ERRO: " << errorCount << endl;
  return 0;
}
