/*
 * main.cpp
 *
 *  Created on: 2016/6/29
 *      Author: Tsukasa Fukunaga
 *
 *  Modified by Goro Terai on: 2022/1/17
 *
 */

#include <string>
#include <stdlib.h>
#include <iostream>
#include "CapR.h"
#include "Probing.h"
#include <getopt.h>

using namespace std;

int main(int argc, char* argv[]) {

  // optionのparse
  // https://qiita.com/tobira-code/items/24e583c30f07c4853f8f
  //int c, flag = 2;
  int c;
  const char* optstring = "" ; // optstringを定義します
  
  const struct option longopts[] = {
    //{    *name,           has_arg, *flag, val },
    //{  "create",       no_argument,     0, 'c' },
    {  "SHAPE", required_argument,              0, 's' },
    {  "SHAPEslope", required_argument,         0, 'l' },
    {  "SHAPEintercept", required_argument,     0, 'i' },
    {  "DMS", required_argument,                0, 'd' }, 
    //{ "setflag",       no_argument, &flag,  1  },
    //{ "clrflag",       no_argument, &flag,  0  },
    {         0,                 0,     0,  0  }, // termination
  };
  int longindex = 0;
  
  opterr = 1; 
  
  // non-option or end of argument list or error('?')までloop
  //Probing *probing = new Probing();
  Probing probing;
  while ((c=getopt_long(argc, argv, optstring, longopts, &longindex)) != -1) {
    if (c == 's' || c == 'l' || c == 'i' || c == 'd') {
      const struct option* longopt = &longopts[longindex];
      printf("longopt=%s ", longopt->name);
      if (longopt->has_arg == required_argument) {
	printf("optarg=%s ", optarg);
      }
      printf("\n");
      if(longopt->name == "SHAPEintercept"){
	probing.shape_slope = atof(optarg);
      }
      if(longopt->name == "SHAPE"){
	probing.shape_file = optarg;
	probing.shape_flag = 1;
      }
      if(longopt->name == "DMS"){
	probing.shape_file = optarg;
	probing.dms_flag = 1;
      }
      if(longopt->name == "SHAPEslope"){
	probing.shape_slope = atof(optarg);
      }
    }
    else{
      return -1;
    }
  }

  if(probing.shape_flag == 1 && probing.dms_flag == 1){
    fprintf(stderr, "Either --SHAPE or --DMS must be used.\n");
    exit(1);
  }
  else if(probing.shape_flag == 1 || probing.dms_flag == 1){
    probing.readReact(probing.shape_file);
  }
  
  //printf("optind=%d, argc=%d\n", optind, argc);
  
  if(argc - optind != 3){
    cout << "Error: The number of argument is invalid." << endl;
    return(0);
  }
  
  string input_file = argv[optind];
  string output_file = argv[optind+1];
  int constraint =  atoi(argv[optind+2]);
  CapR capr(input_file, output_file, constraint, probing);
  capr.Run();
  
  return(0);
}
