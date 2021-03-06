/*
 * CapR.cpp
 *
 *  Created on: 2016/6/29
 *      Author: Tsukasa Fukunaga
 *  Modified by Goro Terai on: 2022/1/18
 *  
 */

#include "CapR.h"
#include "fastafile_reader.h"

void CapR::Run(){
  vector<string> sequences; sequences.reserve(10);
  vector<string> names; names.reserve(10);
  
  FastafileReader fastafile_reader;
  fastafile_reader.ReadFastafile(_input_file_name, sequences, names);
  
  for (int i = 0; i < sequences.size() ; i++){
    //cout << probing.shape_flag << endl;
    //cout << probing.norm_react.count(names[i]) << endl;
    cout << "processing: " << names[i] << endl;
    
    p_energy.clear();
    
    if((probing.shape_flag == 1 || probing.dms_flag == 1)){
      if(probing.norm_react.count(names[i]) != 1){
	//各配列に対応したreactivityのデータがあるかどうかをチェック
	fprintf(stderr, "SHAPE or DMS reactivity data for %s is missing.\n", names[i].c_str());
	exit(1);
      }
      else{
	calcPE(sequences[i], names[i], probing.shape_flag, probing.dms_flag);
      }
    }
    else{
      zeroPE(sequences[i]);
    }
    CalcMain(sequences[i],names[i]);
  }
}

void CapR::calcPE(string seq, string sname, int shape_flag, int dms_flag){
  if(shape_flag == 1 && dms_flag == 0){
    calcSHAPE_PE(seq, sname);
  }
  else if(shape_flag == 0 && dms_flag == 1){
    calcDMS_PE(seq, sname);
  }
  else{
    fprintf(stderr, "Either shape_flag (%d) or dms_flag (%d) must be 1.", shape_flag, dms_flag);
    exit(1);
  }
}

void CapR::calcSHAPE_PE(string seq, string sname){
  //fprintf(stderr, "calculating SHAPE pseudo-energy\n");
  vector<double> n_react = probing.norm_react[sname];
  double pe;
  for(int i = 0; i < seq.size(); i++){
    if(n_react[i] <= -500){
      pe = 0;
    }
    else if(n_react[i] <= 0){
      pe = probing.shape_intercept;
    }
    else{
      pe = log(n_react[i]+1.0) * probing.shape_slope + probing.shape_intercept;
    }
    p_energy.push_back(pe);
  }
  for(int i = 0; i < seq.size(); i++){
    cout << i << " " << n_react[i] << " " << p_energy[i] << endl;
  }
}

double CapR::calcMixGamma(double value, vector <double> param){
  double g1 = 0;
  double g2 = 0;
  
  g1 = calcGamma(param[0], value - param[1], param[2]);
  g2 = calcGamma(param[3], value - param[4], param[5]);
  //printf("value=%f\n", value);
  //printf("Gammadist1=%f\n", g1);
  //printf("Gammadist2=%f\n", g2);

  return g1 * param[6] + g2 * param[7];
  
}

double CapR::calcGamma(double shape, double x, double scale){
  //printf("%f\n", pow(scale, shape)/tgamma(x)) * pow(x, (shape - 1)) * exp(-scale * x);
  //printf("%f %f\n", tgamma(x), x);
  double lam = 1/scale;
  return (pow(lam, shape)/tgamma(shape)) * pow(x, (shape - 1)) * exp(-lam * x);
  //return (pow(scale, shape)/tgamma(shape)) * pow(x, (shape - 1)) * exp(-scale * x);
  //return (1/scale)*pow((x)*(1/scale), (shape - 1))*exp(-(1/scale)*(x))/tgamma(shape);
}

void CapR::calcDMS_PE(string seq, string sname){
  //fprintf(stderr, "calculating DMS pseudo-energy\n");
  vector<double> n_react = probing.norm_react[sname];
  double pe;

  for(int i = 0; i < seq.size(); i++){
    double prob_p = calcMixGamma(n_react[i], probing.DMSparam_pair);
    double prob_u = calcMixGamma(n_react[i], probing.DMSparam_unpair);
    //printf("pairP=%f, unpairP=%f, p_energy=%f, kT=%f\n", prob_p, prob_u, -probing.kT*log(prob_p/prob_u), probing.kT);
    double pe = -probing.kT*log(prob_p/prob_u)/10; // shapeと単位を合わせるため。x(100cal/mol)の単位をx(kcal/mol)に変換する。
    
    
    if(isnan(pe)){
      pe = 0;
    }
    p_energy.push_back(pe);
    
    cout << i << " " << n_react[i] << " " << p_energy[i] << endl;
  }
  /*
  for(int i = 0; i < seq.size(); i++){
    p_energy.push_back(0);
  }
  */
}


void CapR::zeroPE(string seq){
  for(int i = 0; i < seq.size(); i++){
    p_energy.push_back(0);
  }
}

void CapR::CalcMain(string &sequence, string &name){
  Initiallize(sequence);
  CalcInsideVariable();
  CalcOutsideVariable();
  CalcStructuralProfile(name);
  Clear();
}

void CapR::Initiallize(string &sequence){
  _seq_length = sequence.length();
  _int_sequence.resize(_seq_length+1);
  for(int i = 0;i < _seq_length;i++){
    if(sequence[i] == 'A' || sequence[i] == 'a'){
      _int_sequence[i+1] = 1;
    }else if(sequence[i] == 'C' || sequence[i] == 'c'){
      _int_sequence[i+1] = 2;
    }else if(sequence[i] == 'G' || sequence[i] == 'g'){
      _int_sequence[i+1] = 3;
    }else if(sequence[i] == 'T' || sequence[i] == 't' || sequence[i] == 'U' || sequence[i] == 'u'){
      _int_sequence[i+1] = 4;
    }else{
      _int_sequence[i+1] = 0;
    }
  }
  
  _Alpha_outer.resize(_seq_length+1, 0);
  _Alpha_stem.resize(_seq_length+1,vector<double>(_maximal_span+2, -INF));
  _Alpha_stemend.resize(_seq_length+1,vector<double>(_maximal_span+2, -INF));
  _Alpha_multi.resize(_seq_length+1,vector<double>(_maximal_span+2, -INF));
  _Alpha_multibif.resize(_seq_length+1,vector<double>(_maximal_span+2, -INF));
  _Alpha_multi1.resize(_seq_length+1,vector<double>(_maximal_span+2, -INF));
  _Alpha_multi2.resize(_seq_length+1,vector<double>(_maximal_span+2, -INF));
  

  _Beta_outer.resize(_seq_length+1, 0);
  _Beta_stem.resize(_seq_length+1,vector<double>(_maximal_span+2, -INF));
  _Beta_stemend.resize(_seq_length+1,vector<double>(_maximal_span+2, -INF));
  _Beta_multi.resize(_seq_length+1,vector<double>(_maximal_span+2, -INF));
  _Beta_multibif.resize(_seq_length+1,vector<double>(_maximal_span+2, -INF));
  _Beta_multi1.resize(_seq_length+1,vector<double>(_maximal_span+2, -INF));
  _Beta_multi2.resize(_seq_length+1,vector<double>(_maximal_span+2, -INF));
}

void CapR::CalcInsideVariable(){
  //for (int i = 0; i <= _seq_length; i++){
  //  std::cout << _int_sequence[i] << std::endl;
  //}
  for (int j =TURN+1; j <= _seq_length; j++){
    for (int i=j-TURN; i >= max(0,j-_maximal_span-1); i--){
      //std::cout << _int_sequence[i+1] << ":" << _int_sequence[j] << std::endl;
      //dispMat(1,2);
      //Alpha_stem
      int type = BP_pair[_int_sequence[i+1]][_int_sequence[j]];
      int type2 = BP_pair[_int_sequence[i+2]][_int_sequence[j-1]];
      //std::cout << i << "," << j << std::endl;
      double temp = 0; bool flag = 0;
      double temp_pe = 0;
      if (type != 0) {
	type2 = rtype[type2];
	if(_Alpha_stem[i+1][j-i-2] != -INF){
	  //Stem¨Stem
	  if(type2 != 0){
	    temp = _Alpha_stem[i+1][j-i-2]+ LoopEnergy(type, type2,i+1,j,i+2,j-1);
	    //temp = _Alpha_stem[i+1][j-i-2]+ LoopEnergy(type, type2,i+1,j,i+2,j-1)
	    //  + p_energy[i+1] + p_energy[j] + p_energy[i+2] + p_energy[j-1]; // stemの時はpeが2回カウントされる。
	  }
	  flag = 1;
	}
	
	if(_Alpha_stemend[i+1][j-i-2] != -INF){
	  //Stem¨StemEnd
	  temp = flag == 1 ? logsumexp(temp,_Alpha_stemend[i+1][j-i-2]) : _Alpha_stemend[i+1][j-i-2];
	  //temp_pe = _Alpha_stemend[i+1][j-i-2] + p_energy[i+1] + p_energy[j]; // closing hairpin, bulge, internal, multi
	  //temp = flag == 1 ? logsumexp(temp, temp_pe) : temp_pe;
	  flag = 1;
	}
	
	_Alpha_stem[i][j-i] = flag == 0 ? -INF : temp;
      }else{
	_Alpha_stem[i][j-i] = -INF;
      }
      //std::cout << "CHK" << i << "," << j << " " <<  _Alpha_stem[i][j-i] << "pair=" << _int_sequence[i+1] << ":" << _int_sequence[j] << std::endl;
      
      //Alpha_multiBif
      temp = 0; flag = 0;
      for (int k=i+1; k<=j-1; k++){
	if(_Alpha_multi1[i][k-i] != -INF && _Alpha_multi2[k][j-k] != -INF){
	  temp = flag == 0 ? _Alpha_multi1[i][k-i]+_Alpha_multi2[k][j-k] : logsumexp(temp,_Alpha_multi1[i][k-i]+_Alpha_multi2[k][j-k]);
	  flag = 1;
	}
      }
      _Alpha_multibif[i][j-i] = flag == 0 ? -INF : temp;
      
      //Alpha_multi2
      temp = 0; flag = 0; 
      if (type != 0) {
	if(_Alpha_stem[i][j-i] != -INF){
	  temp = _Alpha_stem[i][j-i]+MLintern+CalcDangleEnergy(type,i,j);
	  flag = 1;
	}
      }
      if(_Alpha_multi2[i][j-i-1] != -INF){
	_Alpha_multi2[i][j-i] = _Alpha_multi2[i][j-i-1]+MLbase;
	if(flag == 1){
	  _Alpha_multi2[i][j-i] = logsumexp(temp,_Alpha_multi2[i][j-i]);
	}
      }else{
	_Alpha_multi2[i][j-i] = flag == 0 ? -INF : temp;
      }
      
      //Alpha_multi1
      if(_Alpha_multi2[i][j-i] != -INF && _Alpha_multibif[i][j-i] != -INF){
	_Alpha_multi1[i][j-i] = logsumexp(_Alpha_multi2[i][j-i],_Alpha_multibif[i][j-i]);
      }else if(_Alpha_multi2[i][j-i] == -INF){
	_Alpha_multi1[i][j-i] = _Alpha_multibif[i][j-i];
      }else if(_Alpha_multibif[i][j-i] == -INF){
	_Alpha_multi1[i][j-i] = _Alpha_multi2[i][j-i];
      }else{
	_Alpha_multi1[i][j-i] = -INF;
      }
      
      //Alpha_multi
      flag = 0;
      if(_Alpha_multi[i+1][j-i-1] != -INF){
	_Alpha_multi[i][j-i] = _Alpha_multi[i+1][j-i-1]+MLbase;
	flag = 1;
      }
      
      if(flag == 1){
	if(_Alpha_multibif[i][j-i] != -INF){
	  _Alpha_multi[i][j-i] = logsumexp(_Alpha_multi[i][j-i],_Alpha_multibif[i][j-i]);
	}
      }else{
	_Alpha_multi[i][j-i] = _Alpha_multibif[i][j-i];
      }
      
      //Alpha_stemend
      if(j != _seq_length){
	temp = 0;
	type = BP_pair[_int_sequence[i]][_int_sequence[j+1]];
	if (type!=0) {
	  //StemEnd¨sn
	  temp = HairpinEnergy(type, i,j+1);
	  
	  //StemEnd¨sm_Stem_sn
	  for (int p =i; p <= min(i+MAXLOOP,j-TURN-2); p++) {
	    int u1 = p-i;
	    for (int q=max(p+TURN+2,j-MAXLOOP+u1); q<=j; q++) {
	      type2 = BP_pair[_int_sequence[p+1]][_int_sequence[q]];
	      if(_Alpha_stem[p][q-p] != -INF){
		if (type2 != 0 && !(p == i && q == j)) {
		  type2 = rtype[type2];
		  temp = logsumexp(temp,_Alpha_stem[p][q-p]+LoopEnergy(type, type2,i,j+1,p+1,q)); 
		}
	      }
	    }
	  }
	  
	  //StemEnd¨Multi
	  int tt = rtype[type];
	  temp = logsumexp(temp,_Alpha_multi[i][j-i]+MLclosing+MLintern+dangle3[tt][_int_sequence[i+1]]+dangle5[tt][_int_sequence[j]]);
	  _Alpha_stemend[i][j-i] = temp;
	}else{
	  _Alpha_stemend[i][j-i] = -INF;
	}
      }
    }
  }
  
  //Alpha_Outer
  for(int i = 1;i <= _seq_length;i++){
    double temp = _Alpha_outer[i-1];
    for(int p = max(0,i-_maximal_span-1); p <i;p++){
      if(_Alpha_stem[p][i-p] != -INF){
	int type = BP_pair[_int_sequence[p+1]][_int_sequence[i]];
	double ao = _Alpha_stem[p][i-p]+CalcDangleEnergy(type,p,i);
	temp = logsumexp(temp,ao+_Alpha_outer[p]);
      }
    }
    _Alpha_outer[i] = temp;
  }
}

void CapR::CalcStructuralProfile(string &name){
  vector<double> bulge_probability; bulge_probability.resize(_seq_length, 0.0);
  vector<double> internal_probability; internal_probability.resize(_seq_length, 0.0);
  vector<double> hairpin_probability; hairpin_probability.resize(_seq_length, 0.0);
  vector<double> multi_probability; multi_probability.resize(_seq_length, 0.0);
  vector<double> exterior_probability; exterior_probability.resize(_seq_length, 0.0);
  vector<double> stem_probability; stem_probability.resize(_seq_length, 0.0);
  vector<double> stemL_probability; stemL_probability.resize(_seq_length, 0.0);
  vector<double> stemR_probability; stemR_probability.resize(_seq_length, 0.0);

  double pf = _Alpha_outer[_seq_length];
  //printf("part_func=%.2f\n", -pf*kT/1000);
  if(pf >= -690 && pf <= 690){
    CalcBulgeAndInternalProbability(bulge_probability, internal_probability);
  }else{
    CalcLogSumBulgeAndInternalProbability(bulge_probability, internal_probability);}
  
  CalcHairpinProbability(hairpin_probability);
  //printf("%d %d\n", _seq_length, _maximal_span);
  CalcStemProbability(stemL_probability, stemR_probability);
  
  for(int i = 1; i <=_seq_length;i++){
    exterior_probability[i-1] = CalcExteriorProbability(i);
    multi_probability[i-1] = CalcMultiProbability(i);
    //stemL_probability[i-1] = CalcStemProbabilityL(i);
    stem_probability[i-1] = 1.0 - bulge_probability[i-1] - exterior_probability[i-1] - hairpin_probability[i-1] - internal_probability[i-1] - multi_probability[i-1];
  }


  for(int i = 0; i <_seq_length;i++){
    double chk = stemL_probability[i] + stemR_probability[i];
    if(abs(stem_probability[i] - chk) >= 1e-5){
      fprintf(stderr, "stem probabilities mismatch (org=%f, calc=%f)\n",stem_probability[i], chk);
      exit(0);
    }
  }
  
  ofstream of(_output_file_name.c_str(), ios::out | ios::app);
  of << ">" + name << endl;
  //  of << "Bulge ";
  of << "bulge ";
  for(int i = 0; i <_seq_length;i++){
    of << bulge_probability[i]<<" ";
  }
  of << endl;
  //  of << "Exterior ";
  of << "external ";
  for(int i = 0; i <_seq_length;i++){
    //of << exterior_probability[i]<<" ";
    of << exterior_probability[i] + multi_probability[i] <<" ";
  }
  of << endl;
  //of << "Hairpin ";
  of << "hairpin ";
  for(int i = 0; i <_seq_length;i++){
    of << hairpin_probability[i]<<" ";
  }
  of << endl;
  //of << "Internal ";
  of << "internal ";
  for(int i = 0; i <_seq_length;i++){
    of << internal_probability[i]<<" ";
  }
  of << endl;
  //of << "Multibranch ";
  //for(int i = 0; i <_seq_length;i++){
  //  of << multi_probability[i]<<" ";
  //}
  //of << endl;
  //of << "Stem ";
  //for(int i = 0; i <_seq_length;i++){
  //  of << stem_probability[i]<<" ";
  //}
  //of << endl;
  of << "dspL ";
  for(int i = 0; i <_seq_length;i++){
    of << stemL_probability[i] <<" ";
  }
  of << endl;
  of << "dspR ";
  for(int i = 0; i <_seq_length;i++){
    of << stemR_probability[i] <<" ";
  }
  of << endl;
  of << endl;
  of.close();
}

double CapR::CalcExteriorProbability(int x){
  double probability = exp(_Alpha_outer[x-1]+_Beta_outer[x]-_Alpha_outer[_seq_length]);
  return(probability);
}

void CapR::CalcStemProbability(vector<double> & stemL_probability, vector<double> & stemR_probability){
  for(int i = 1; i <= _seq_length; i++){
    for(int j = i+1; j<=min(_seq_length, i+_maximal_span); j++){
      double temp = 0.0;
      bool flag = 0;
      int type = 0;
      int type2 = 0;
      double stem_pf = 0.0;
      double stemend_pf = 0.0;
      double prob = 0.0;
      
      if(_Beta_stem[i-1][j-i+1] != -INF && _Alpha_stemend[i][j-1-i] != -INF){
	stemend_pf = _Beta_stem[i-1][j-i+1] + _Alpha_stemend[i][j-1-i];
	temp = flag == 1 ? logsumexp(temp, stemend_pf) : stemend_pf;
	flag = 1;
      }
      if(_Beta_stem[i-1][j-i+1] != -INF && _Alpha_stem[i][j-1-i] != -INF){
	type = BP_pair[_int_sequence[i]][_int_sequence[j]];
	if (type!=0) {
	  type2 = BP_pair[_int_sequence[i+1]][_int_sequence[j-1]];
	  if (type2 != 0) {
	    type2 = rtype[type2];
	    if(_Beta_stem[i-1][j-i+1] != -INF && _Alpha_stem[i][j-1-i] != -INF){
	      stem_pf = _Beta_stem[i-1][j-i+1] + _Alpha_stem[i][j-1-i] + LoopEnergy(type, type2,i,j,i+1,j-1);
	      temp = flag == 1 ? logsumexp(temp, stem_pf) : stem_pf;
	      flag = 1;
	    }
	    
	  }
	}
      } 

      if(flag == 1){
	prob = exp(temp-_Alpha_outer[_seq_length]);
	//std::cout << i << " " << j << " PF=" << temp << " probL=" << prob << std::endl;
      }
      else{
	prob = 0.0;
      }
      stemL_probability[i-1] += prob;
      stemR_probability[j-1] += prob;
    }
    
  }
  
}

double CapR::CalcStemProbabilityL(int i){
  double temp = 0.0;
  bool flag = 0;
  int type = 0;
  int type2 = 0;
  double stem_pf = 0.0;
  double stemend_pf = 0.0;
  double prob = 0.0;
  for(int j = i+1; j<=min(_seq_length, i+_maximal_span); j++){
    
    if(_Beta_stem[i-1][j-i+1] != -INF && _Alpha_stemend[i][j-1-i] != -INF){
      stemend_pf = _Beta_stem[i-1][j-i+1] + _Alpha_stemend[i][j-1-i];
      temp = flag == 1 ? logsumexp(temp, stemend_pf) : stemend_pf;
      flag = 1;
    }
    if(_Beta_stem[i-1][j-i+1] != -INF && _Alpha_stem[i][j-1-i] != -INF){
      type = BP_pair[_int_sequence[i]][_int_sequence[j]];
      if (type!=0) {
	type2 = BP_pair[_int_sequence[i+1]][_int_sequence[j-1]];
	if (type2 != 0) {
	  type2 = rtype[type2];
	  if(_Beta_stem[i-1][j-i+1] != -INF && _Alpha_stem[i][j-1-i] != -INF){
	    stem_pf = _Beta_stem[i-1][j-i+1] + _Alpha_stem[i][j-1-i] + LoopEnergy(type, type2,i,j,i+1,j-1);
	    temp = flag == 1 ? logsumexp(temp, stem_pf) : stem_pf;
	    flag = 1;
	  }
	  
	}
      }
    } 
  }

  if(flag == 1){
    prob = exp(temp-_Alpha_outer[_seq_length]);
  }
  else{
    prob = 0.0;
  }
  //std::cout << i << " " << " PF=" << temp << " probL=" << prob << std::endl;
  //return(probability);
  return prob;
}


void CapR::CalcHairpinProbability(vector<double> &hairpin_probability){
  for(int x = 1; x <=_seq_length;x++){
    double temp = 0.0;
    int type = 0;
    bool flag = 0;
    double h_energy = 0.0;
    
    for(int i = max(1,x-_maximal_span);i<x ;i++){
      for(int j = x+1; j<=min(i+_maximal_span,_seq_length);j++){
	type = BP_pair[_int_sequence[i]][_int_sequence[j]];
	if(_Beta_stemend[i][j-i-1] != -INF){
	  h_energy = _Beta_stemend[i][j-i-1] + HairpinEnergy(type, i,j);
	  temp = flag == 1 ? logsumexp(temp, h_energy) : h_energy;
	  flag = 1;
	}
      }
    }

    if(flag == 1){
      hairpin_probability[x-1] = exp(temp-_Alpha_outer[_seq_length]);
    }else{
      hairpin_probability[x-1] = 0.0;
    }
  }
}

double CapR::CalcMultiProbability(int x){
  double probability = 0.0;
  double temp = 0.0;
  bool flag = 0;
  
  for(int i = x; i<=min(x+_maximal_span,_seq_length);i++){
    if(_Beta_multi[x-1][i-x+1] != -INF && _Alpha_multi[x][i-x] != -INF){
      temp = flag == 0 ? _Beta_multi[x-1][i-x+1] + _Alpha_multi[x][i-x] : logsumexp(temp,_Beta_multi[x-1][i-x+1] + _Alpha_multi[x][i-x]);
      flag = 1;
    }
  }
  
  for(int i = max(0,x-_maximal_span); i<x;i++){
    if(_Beta_multi2[i][x-i] != -INF && _Alpha_multi2[i][x-i-1] != -INF){
      temp = flag == 0 ? _Beta_multi2[i][x-i] + _Alpha_multi2[i][x-i-1] : logsumexp(temp,_Beta_multi2[i][x-i] + _Alpha_multi2[i][x-i-1]);
      flag = 1;
    }
  }
  if(flag == 1){ probability = exp(temp-_Alpha_outer[_seq_length]); }
  return(probability);
}

void CapR::CalcBulgeAndInternalProbability(vector<double> &bulge_probability, vector<double> &internal_probability){
  double temp = 0;
  int type = 0;
  int type2 = 0;
 
  for(int i = 1; i<_seq_length-TURN-2;i++){
    for(int j = i+TURN+3; j<=min(i+_maximal_span,_seq_length);j++){
      type = BP_pair[_int_sequence[i]][_int_sequence[j]];
      if (type!=0) {
	for (int p =i+1; p <= min(i+MAXLOOP+1,j-TURN-2); p++) {
	  int u1 = p-i-1;
	  for (int q=max(p+TURN+1,j-MAXLOOP+u1-1); q<j; q++) {
	    type2 = BP_pair[_int_sequence[p]][_int_sequence[q]];
	    if (type2 != 0 && !(p == i+1 && q == j-1)) {
	      type2 = rtype[type2];
	      if(_Beta_stemend[i][j-i-1] != -INF && _Alpha_stem[p-1][q-p+1] != -INF){
		temp = exp(_Beta_stemend[i][j-i-1] + LoopEnergy(type, type2,i,j,p,q)+_Alpha_stem[p-1][q-p+1]);
		
		for(int k = i+1; k <= p-1;k++){
		  if(j == q+1){
		    bulge_probability[k-1] += temp;		   
		  }else{
		    internal_probability[k-1] += temp;		  
		  }
		}
		
		for(int k = q+1; k <= j-1;k++){
		  if(i == p-1){
		    bulge_probability[k-1] += temp;		   
		  }else{
		    internal_probability[k-1] += temp;		  
		  }
		}
	      } 
	    }
	  }
	}
      }
    }
  }
  
  for(int i=0;i<_seq_length;i++){
    if(bulge_probability[i] != 0){
      bulge_probability[i] /= exp(_Alpha_outer[_seq_length]);
    }
    if(internal_probability[i] != 0){
      internal_probability[i] /= exp(_Alpha_outer[_seq_length]);
    }
  }
}

void CapR::CalcLogSumBulgeAndInternalProbability(vector<double> &bulge_probability, vector<double> &internal_probability){
  double probability = 0;
  double temp = 0;
  int type = 0;
  int type2 = 0;
  vector<bool> b_flag_array; b_flag_array.resize(_seq_length,0);
  vector<bool> i_flag_array; i_flag_array.resize(_seq_length,0);
  
  for(int i = 1; i<_seq_length-TURN-2;i++){
    for(int j = i+TURN+3; j<=min(i+_maximal_span,_seq_length);j++){
      type = BP_pair[_int_sequence[i]][_int_sequence[j]];
      if (type!=0) {
	for (int p =i+1; p <= min(i+MAXLOOP+1,j-TURN-2); p++) {
	  int u1 = p-i-1;
	  for (int q=max(p+TURN+1,j-MAXLOOP+u1-1); q<j; q++) {
	    type2 = BP_pair[_int_sequence[p]][_int_sequence[q]];
	    if (type2 != 0 && !(p == i+1 && q == j-1)) {
	      type2 = rtype[type2];
	      if(_Beta_stemend[i][j-i-1] != -INF && _Alpha_stem[p-1][q-p+1] != -INF){
		temp = _Beta_stemend[i][j-i-1] + LoopEnergy(type, type2,i,j,p,q)+_Alpha_stem[p-1][q-p+1];
		
		for(int k = i+1; k <= p-1;k++){
		  if(j == q+1){
		    bulge_probability[k-1] = b_flag_array[k-1] == 1 ? logsumexp(bulge_probability[k-1], temp) : temp;
		    b_flag_array[k-1] = 1;		   
		  }else{
		    internal_probability[k-1] = i_flag_array[k-1] == 1 ? logsumexp(internal_probability[k-1], temp) : temp;
		    i_flag_array[k-1] = 1;		  
		  }
		}
		
		for(int k = q+1; k <= j-1;k++){
		  if(i == p-1){
		    bulge_probability[k-1] = b_flag_array[k-1] == 1 ? logsumexp(bulge_probability[k-1], temp) : temp;
		    b_flag_array[k-1] = 1;
		  }else{
		    internal_probability[k-1] = i_flag_array[k-1] == 1 ? logsumexp(internal_probability[k-1], temp) : temp;
		    i_flag_array[k-1] = 1;
		  }
		}
	      } 
	    }
	  }
	}
      }
    }
  }
  
  for(int i=0;i<_seq_length;i++){
    if(b_flag_array[i]==1){
      bulge_probability[i] = exp(bulge_probability[i]-_Alpha_outer[_seq_length]);
    }
    if(i_flag_array[i]==1){
      internal_probability[i] = exp(internal_probability[i]-_Alpha_outer[_seq_length]);
    }
  }
}

double CapR::CalcDangleEnergy(int type, int a, int b){
  double x = 0;
  if (type != 0) {
    if (a>0) x += dangle5[type][_int_sequence[a]];
    if (b<_seq_length) x += dangle3[type][_int_sequence[b+1]];
    if( b == _seq_length && type>2){
      x += TermAU;
    }
  }
  return(x);
}

void CapR::CalcOutsideVariable(){
  //Beta_outer
  for(int i = _seq_length-1;i >= 0;i--){
    double temp = _Beta_outer[i+1];
    for(int p = i+1; p <= min(i+_maximal_span+1,_seq_length);p++){
      if(_Alpha_stem[i][p-i] != -INF){
	int type = BP_pair[_int_sequence[i+1]][_int_sequence[p]];
	double bo = _Alpha_stem[i][p-i] + CalcDangleEnergy(type,i,p);
	temp = logsumexp(temp,bo+_Beta_outer[p]);
      }
    }
    _Beta_outer[i] = temp;	
  }
  
  for (int q=_seq_length; q>=TURN+1; q--) {
    for (int p=max(0,q-_maximal_span-1); p<= q-TURN; p++) {
      int type = 0;
      int type2 = 0;

      double temp = 0; bool flag = 0;
      if(p != 0 && q != _seq_length){
	//Beta_stemend
	_Beta_stemend[p][q-p] = q-p >= _maximal_span ? -INF : _Beta_stem[p-1][q-p+2];
	
	//Beta_Multi
	flag = 0;
	if(q-p+1 <= _maximal_span+1){
	  if(_Beta_multi[p-1][q-p+1] != -INF){
	    temp = _Beta_multi[p-1][q-p+1] + MLbase;
	    flag = 1;
	  }
	}
	
	type = BP_pair[_int_sequence[p]][_int_sequence[q+1]];
	int tt = rtype[type];
	if(flag == 1){
	  if(_Beta_stemend[p][q-p] != -INF){
	    temp = logsumexp(temp,_Beta_stemend[p][q-p]+MLclosing+MLintern+ dangle3[tt][_int_sequence[p+1]]+dangle5[tt][_int_sequence[q]]);
	  }
	}else{
	  if(_Beta_stemend[p][q-p] != -INF){
	    temp = _Beta_stemend[p][q-p]+MLclosing+MLintern+dangle3[tt][_int_sequence[p+1]]+dangle5[tt][_int_sequence[q]];
	  }else{
	    temp = -INF;
	  }
	}
	_Beta_multi[p][q-p] = temp;
	
	//Beta_Multi1
	temp = 0; flag = 0;
	for(int k = q+1 ; k<= min(_seq_length,p+_maximal_span);k++){
	  if(_Beta_multibif[p][k-p] != -INF && _Alpha_multi2[q][k-q] != -INF){
	    temp = flag == 0 ? _Beta_multibif[p][k-p]+_Alpha_multi2[q][k-q] : logsumexp(temp,_Beta_multibif[p][k-p]+_Alpha_multi2[q][k-q]) ;
	    flag = 1;
	  }
	}
	_Beta_multi1[p][q-p] = flag == 1 ? temp: -INF;
	
	//Beta_Multi2
	temp = 0; flag = 0;
	if(_Beta_multi1[p][q-p] != -INF){
	  temp = _Beta_multi1[p][q-p];
	  flag = 1;
	}
	if(q-p <= _maximal_span){
	  if(_Beta_multi2[p][q-p+1] != -INF){
	    temp = flag == 1 ? logsumexp(temp,_Beta_multi2[p][q-p+1]+MLbase) : _Beta_multi2[p][q-p+1]+MLbase;
	    flag = 1;
	  }
	}
	
	for(int k = max(0,q-_maximal_span); k < p ;k++){
	  if(_Beta_multibif[k][q-k] != -INF && _Alpha_multi1[k][p-k] != -INF){
	    temp = flag == 0 ? _Beta_multibif[k][q-k]+_Alpha_multi1[k][p-k] : logsumexp(temp,_Beta_multibif[k][q-k]+_Alpha_multi1[k][p-k]);
	    flag = 1;
	  }
	}
	_Beta_multi2[p][q-p] = flag == 0 ? -INF : temp;
	
	//Beta_multibif
	if(_Beta_multi1[p][q-p] != -INF && _Beta_multi[p][q-p] != -INF){
	  _Beta_multibif[p][q-p] = logsumexp(_Beta_multi1[p][q-p],_Beta_multi[p][q-p]);
	}else if(_Beta_multi[p][q-p] == -INF){
	  _Beta_multibif[p][q-p] = _Beta_multi1[p][q-p];
	}else if(_Beta_multi1[p][q-p] == -INF){
	  _Beta_multibif[p][q-p] = _Beta_multi[p][q-p];
	}else{
	  _Beta_multibif[p][q-p] = -INF;
	}
	
      }
      
      //Beta_stem
      type2 = BP_pair[_int_sequence[p+1]][_int_sequence[q]];
      if(type2 != 0){
	temp = _Alpha_outer[p]+_Beta_outer[q]+CalcDangleEnergy(type2,p,q);
	type2 = rtype[type2];
	for (int i=max(1,p-MAXLOOP); i<=p; i++){
	  for (int j=q; j<=min(q+ MAXLOOP -p+i,_seq_length-1); j++) {
	    type = BP_pair[_int_sequence[i]][_int_sequence[j+1]];
	    if (type != 0 && !(i == p && j == q)) {
	      if(j-i <= _maximal_span+1 && _Beta_stemend[i][j-i] != -INF){
		temp = logsumexp(temp,_Beta_stemend[i][j-i]+LoopEnergy(type,type2,i,j+1,p+1,q));
	      }
	    }
	  }
	}
	
	if(p != 0 && q != _seq_length){
	  type = BP_pair[_int_sequence[p]][_int_sequence[q+1]];
	  if(type != 0){
	    if(q-p+2 <= _maximal_span+1 && _Beta_stem[p-1][q-p+2] != -INF){
	      temp = logsumexp(temp,_Beta_stem[p-1][q-p+2]+LoopEnergy(type,type2,p,q+1,p+1,q));
	    }
	  }
	}
	_Beta_stem[p][q-p] = temp;
	//std::cout << "TEST:" << _Beta_stem[p][q-p] << std::endl;
	if(_Beta_multi2[p][q-p] != -INF){
	  type2 = rtype[type2];
	  temp = _Beta_multi2[p][q-p] + MLintern + CalcDangleEnergy(type2,p,q);
	  _Beta_stem[p][q-p] = logsumexp(temp,_Beta_stem[p][q-p]);
	}
      }else{
	_Beta_stem[p][q-p] = -INF;
      }
      
      //std::cout << "CHK_BETA\t" << p << "," << q << " " <<  _Beta_stem[p][q-p] << " pair=" << _int_sequence[p+1] << ":" << _int_sequence[q] << "  PF=" << _Alpha_outer[_seq_length] << std::endl;
      //std::cout << "CHK_BETA2\t" << p << "," << q << " " <<  _Beta_stemend[p][q-p] << " pair=" << _int_sequence[p+1] << ":" << _int_sequence[q] << "  PF=" << _Alpha_outer[_seq_length] << std::endl;
      
    }
  }
}

double CapR::logsumexp(double x,double y){
  double temp = x > y ? x + log(exp(y-x) + 1.0) : y + log(exp(x-y) + 1.0) ;
  return(temp);
}



double CapR::LoopEnergy(int type, int type2,int i,int j,int p,int q){
  double z=0;
  int u1 = p-i-1;
  int u2 = j-q-1;
  
  if ((u1==0) && (u2==0)){
    z = stack[type][type2];
    //std::cout << "stack:" << z << std::endl; // eの肩に乗せる直前まで計算してある。単位を揃えて(10倍して）、マイナス１して、ktで割ってある。元々の単位は(10cal/mol)
    z += -(p_energy[i] + p_energy[j] + p_energy[p] + p_energy[q])*1000/kT; // stemの時はpeが2回カウントされる。kcalをcalにするために1000倍している。
  }else{
    if ((u1==0)||(u2==0)) {
      int u;
      u = u1 == 0 ? u2 : u1;
      z = u <=30 ? bulge[u] : bulge[30] - lxc37*log( u/30.)*10./kT;
      
      if (u == 1){
	z += stack[type][type2];
      }else {
	if (type>2){ z += TermAU;}
	if (type2>2){ z += TermAU;}
      }
    }else{     
      if (u1+u2==2) {
	z = int11[type][type2][_int_sequence[i+1]][_int_sequence[j-1]];
      }else if ((u1==1) && (u2==2)){
	z = int21[type][type2][_int_sequence[i+1]][_int_sequence[q+1]][_int_sequence[j-1]];
      }else if ((u1==2) && (u2==1)){
	z = int21[type2][type][_int_sequence[q+1]][_int_sequence[i+1]][_int_sequence[p-1]];
      }else if ((u1==2) && (u2==2)){
	z = int22[type][type2][_int_sequence[i+1]][_int_sequence[p-1]][_int_sequence[q+1]][_int_sequence[j-1]];
      }else{
	z = internal[u1+u2]+mismatchI[type][_int_sequence[i+1]][_int_sequence[j-1]]+mismatchI[type2][_int_sequence[q+1]][_int_sequence[p-1]];
	z += ninio[abs(u1-u2)];
      }
    }
  }
  return z;
}

double CapR::HairpinEnergy(int type, int i, int j) {
  int d = j-i-1;
  double q = 0;

  q = d <= 30 ? hairpin[d] : hairpin[30] - lxc37*log( d/30.) *10./kT;  
  if(d!= 3){
    q += mismatchH[type][_int_sequence[i+1]][_int_sequence[j-1]];
  }else{
    if(type > 2){q += TermAU;}
  }
  return q;
}

void CapR::Clear(){
  for(int i = 0; i <= _seq_length;i++){
    _Alpha_stem[i].clear();
    _Alpha_stemend[i].clear();
    _Alpha_multi[i].clear();
    _Alpha_multibif[i].clear();
    _Alpha_multi1[i].clear();
    _Alpha_multi2[i].clear();
    _Beta_stem[i].clear();
    _Beta_stemend[i].clear();
    _Beta_multi[i].clear();
    _Beta_multibif[i].clear();
    _Beta_multi1[i].clear();
    _Beta_multi2[i].clear();
  }

  _int_sequence.clear();
  _seq_length = 0;
  _Alpha_outer.clear();
  _Alpha_stem.clear();
  _Alpha_stemend.clear();
  _Alpha_multi.clear();
  _Alpha_multibif.clear();
  _Alpha_multi1.clear();
  _Alpha_multi2.clear();
  
  _Beta_outer.clear();
  _Beta_stem.clear();
  _Beta_stemend.clear();
  _Beta_multi.clear();
  _Beta_multibif.clear();
  _Beta_multi1.clear();
  _Beta_multi2.clear();
}
