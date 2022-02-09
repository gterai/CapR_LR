/*
 * Probing.cpp
 *
 *  Created on: 2022/1/17
 *      Author: Goro Terai
 */

#include "Probing.h"
using namespace std;

void Probing::readReact(string &input_file){
  ifstream fp;
  string buffer;
  fp.open(input_file.c_str(), ios::in);
  if (!fp){
    cout << "Error: can't open input_file:"+input_file+"." <<endl;
    exit(1);
  }
  int pos = 1;
  string sid = "";
  vector<double> tmp_react;
  map <string, vector<double>> testmap;
  while(getline(fp,buffer)){
    //cout << buffer << endl;
    if(buffer[0] == '>'){
      sid = buffer.substr(1,buffer.size()-1);
      //cout << sid << endl;
      tmp_react.clear(); // ベクタを空にする
      pos = 1; // posを１に戻す
    }
    else if(buffer[0] == '/' && buffer[1] == '/'){
      norm_react.insert({sid, tmp_react});
    }
    else{
      vector<string> items = split(buffer);
      tmp_react.push_back(stod(items[1]));
      //cout << tmp_react[tmp_react.size()-1] << endl;
      if(atoi(items[0].c_str()) != pos){
	printf("Unexpected position (%d) in reactivity data of %s\n",
	       pos, sid.c_str());
	exit(1);
      }
      pos++;
    }
  }
  
}

vector<string> Probing::split(string str) {
  int first = 0;
  int last = str.find_first_of('\t');
  
  vector<string> result;
  
  while (first < str.size()) {
    string subStr(str, first, last - first);
    
    result.push_back(subStr);
    
    first = last + 1;
    last = str.find_first_of('\t', first);
    
    if (last == string::npos) {
      last = str.size();
    }
  }
  
  return result;
}

