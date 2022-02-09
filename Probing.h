/*
 * Probing.h
 *
 *  Created on: 2022/1/17
 *      Author: Goro Terai
 */

#ifndef PROBING_H
#define PROBING_H

#include <string>
//#include <stdlib.h>
#include <vector>
//#include <math.h>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using namespace std;

class Probing{
 public:
  string shape_file = "";
  double shape_slope = 1.8;
  double shape_intercept = -0.6;
  int dms_flag = 0;
  int shape_flag = 0;
  
  // shape1, location1, scale1, shape2, location2, scale2, weight1, weight2
  vector <double> DMSparam_pair{9.194, 0.000, 0.001, 1.090, 0.000, 0.113, 0.000, 1.000};
  vector <double> DMSparam_unpair{9.755, 0.000, 0.015, 1.275, 0.000, 0.443, 0.103, 0.897};
  double kT = 5.904976983149999;
  
  map <string, vector<double>> norm_react;
  void readReact(string &input_file);
 private:
  //vector<string> split(string str, char del);
  //vector<string> split(string str, char del);
  vector<string> split(string str);
  //void split2(string str);
  
};

#endif
