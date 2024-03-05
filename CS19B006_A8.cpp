#include <bits/stdc++.h>
using namespace std;

int rightshift(int a,int b){
	return a/pow(2,b);
}


int incount = 0;
int arithinst = 0;
int logicinst = 0;
int dintcount = 0;
int ctincount = 0;
int haltcount = 0;
int t=0;
int tot_stall = 0;
int dstall = 0;
int cstall =0;


vector<int> reg(16,0);
vector<int> isval(16,1);
vector<int> invec(256,0);				
vector<int> darr(256,0);

class Pipeline{
	
	private:
	
	int PC =0;

	int branch = 0;
	int dhaz = 0;
	int IR=0;
	int ALout;
	int LMD;											



	bool halt = false;
	queue<int> fetch;
	queue<vector<int>> decode;
	queue<vector<int>> exec;
	queue<vector<int>> memAcc;							
	queue<vector<int>> wriback;

	void createIR(){
		IR = invec[PC]*256 + invec[PC+1];
	}

	public:
	
	
	void loadDarr(int a, int i){darr[i]=a;}
	void loadInvec(int a, int i){invec[i]=a;}
	void loadReg(int a, int i){reg[i]=a;}
	
	int getDarr(int i){return darr[i];}
	int getReg(int i){return reg[i];}
	
	
	void setBranch(){branch=0;}
	
	void IF();
	void ID();
	void EX();
	void MEM();
	void WB();
	
	bool controlHz(){
		if(branch > 0)													//control hazard
        {
            branch = branch - 1;
            t = t+1;
            return 1;
        }
        
        return 0;
	
	}
	
	bool dataHz(){
		if(dhaz > 0)													//datahazard
        {
            dhaz = dhaz-1;
            t = t + 1;
            return 1;
        }
        
        return 0;
	
	}
	
	bool checkEnd(){
		return halt && fetch.empty() && decode.empty() && exec.empty() && memAcc.empty() && wriback.empty();
	}

};

void Pipeline::IF(){						//fetch										
	if(!halt){
		
		createIR();
		PC += 2;
		fetch.push(IR);
		int key = IR;
		key = rightshift(key,12);
		if(key == 15)
        {
            halt = true;
        }
	}
}


void Pipeline::ID(){										//Decode
	if(!fetch.empty()){
		int instruction = fetch.front();					
		fetch.pop();
		vector<int> op(4);
		int k = 3;
		while(k>=0){
			op.at(k) = instruction%16;
			instruction = rightshift(instruction,4);
			k--;
		}
		if(op.at(0)==15){
			halt = true;
		}
        incount++;
		decode.push(op);
	}
	if(!decode.empty())
    {
        auto op = decode.front();
        int numop;
        if(op[0]<3 || op[0]==4 || op[0]==5 || op[0]==7){
				numop = 3;
			}
		else if(op[0]==6 || op[0]==8){
			numop = 2; 
			}
		else if(op[0]==3 || op[0]==11){
			numop = 1;
			}
		else if(op[0]==9){
			numop = 0;														
			}
			
        switch(numop){

            case 0 :

             if(isval[op[2]] && isval[op[1]]){
                dintcount++;
                vector<int> temp = decode.front();
                temp.push_back(0);
                exec.push(temp);
                decode.pop();
            }
            else{
                dstall= dstall+1;
                tot_stall = tot_stall +1;
                dhaz = 1;
            }

            break;





            case 1 :

            if(isval[op[1]]){
                if(op[0]==3) isval[op[1]] = 0;
                vector<int> temp = decode.front();
                if(op[0]==11){
                    ctincount++;
                    cstall = cstall + 2;
                    tot_stall = tot_stall +  2;
                    branch = 2;
                    if(reg[op[1]]==0){
                        temp.push_back(1);
                        while(!fetch.empty()) fetch.pop();
                        while(!decode.empty()) decode.pop();
                        halt = false;
                    }
                    else temp.push_back(0);
                }
                else
                {
                    arithinst++;
                    temp.push_back(0);
                }
                exec.push(temp);
                if(!decode.empty()) decode.pop();
            }
            else{
                dstall = dstall + 1;
                tot_stall = tot_stall +1;
                dhaz = 1;
            }

            break;
            
            
            

            case 2 :

            if(isval[op[2]]){
                isval[op[1]] = 0;
                vector<int> temp = decode.front();
                temp.push_back(0);
                exec.push(temp);
                if(op[0]!=6){
                    dintcount = dintcount+1;
                } 
                else 
                    logicinst = logicinst+1;
                decode.pop();
            }
            else{
                dstall = dstall+1;
                tot_stall = tot_stall +1;
                dhaz = 1;
            }
            break;
            
            
            

            case 3 :
            if(isval[op[3]] && isval[op[2]]){
                isval[op[1]] = 0;
                vector<int> temp = decode.front();
                temp.push_back(0);
                exec.push(temp);
                if(op[0] >=3){
                    logicinst = logicinst+1;
                }
                else{
                    arithinst = arithinst+1;
                }
                decode.pop();
            }
            else{
                dstall = dstall + 1;
                tot_stall = tot_stall +1;
                dhaz = 1;
            }

            break;
            
            
            

            default :

            vector<int> temp = decode.front();
            if(op[0]==10){                                          
                ctincount++;
                cstall+=2;                                      
                tot_stall += 2;
                branch = 2;
                temp.push_back(1);
                while(!fetch.empty()) fetch.pop();                          
                while(!decode.empty()) decode.pop();
                halt = false;
            }
            else
            {
                haltcount++;
                temp.push_back(0);
            }
            exec.push(temp);
            if(!decode.empty())decode.pop();

            break;
            }
	}
}

void Pipeline::EX(){
    if(!exec.empty()){
        vector<int> execute = exec.front();
        exec.pop();
        if(execute[0] >= 0 && execute[0] <=9){
            if(execute[0]==0) ALout = reg[execute[2]] + reg[execute[3]];	//ADD	
            if(execute[0]==1) ALout = reg[execute[2]] - reg[execute[3]];	//SUB	
            if(execute[0]==2) ALout = reg[execute[2]] * reg[execute[3]];	//MUL	
            if(execute[0]==3) ALout = reg[execute[1]] + 1;					//INC
            if(execute[0]==4) ALout = reg[execute[2]] & reg[execute[3]];	//AND
            if(execute[0]==5) ALout = reg[execute[2]] | reg[execute[3]]; 	//OR
            if(execute[0]==6) ALout = ~reg[execute[2]];						//NOT
            if(execute[0]==7) ALout = reg[execute[2]] ^ reg[execute[3]];	//XOR
            if(execute[0] == 8 || execute[0]==9){							// LOAD/STORE
                
                int X = execute[3];
                if((X & 1<<3) != 0) X-=16;
                ALout = reg[execute[2]] + X;
            }
            vector<int> flag;
            flag.push_back(ALout);
            flag.push_back(execute[1]);

            if(execute[0] == 8 || execute[0]==9) flag.push_back(execute[0]-8);

            memAcc.push(flag);            
        }
        if(execute[0] == 10 || execute[0]==11){
            vector<int> temp;
            if(execute[4]==1){
                int unsignInt = 16*execute[2] + execute[3];;
                if(execute[0]==10) unsignInt = 16*execute[1] + execute[2];				//calculating target address

                int signInt = unsignInt;
                if((unsignInt & 1<<7) != 0) signInt = unsignInt - 256;
                
                PC = PC + 2*signInt;
                memAcc.push(temp);
            }
            else memAcc.push(temp);
        }       
        else if(execute[0] == 15){										//pushing dummy vector halting
            vector<int> temp;
            memAcc.push(temp);
        }
    }
}

void Pipeline::MEM(){
	if(!memAcc.empty()){
		vector<int> m ;
        m = memAcc.front();

		memAcc.pop();
		vector<int> flag;
        int x = m.size();
        switch(x){

            case 3 :
            if(m.at(2)!=1){
                LMD = darr[m[0]];
                flag.push_back(LMD);
                flag.push_back(m[1]);
                flag.push_back(0);    
            }
            else{
                darr[m[0]] = reg[m[1]];
            }

            break;

            case 2 :
            flag = m;
            break;

            default :
            break;

        }
		
		wriback.push(flag);													
	}
}

void Pipeline::WB(){
	if(!wriback.empty()){
		vector<int> flag;
		flag = wriback.front();	
		wriback.pop();
		int x = flag.size();
		switch(x){

            case 2 :
            reg[flag[1]] = flag[0];
            isval[flag[1]] = 1;
            break;

            case 3 :

            reg[flag[1]] = flag[0];
            isval[flag[1]] = 1;
            break;

            default :
            break;
        }
		
	}
}

int main()
{
	
	Pipeline P;
	ifstream DFile("DCache.txt");										//creating an array from Dcache
	string ch;
	int i = 0;
	while(i<256){
		getline(DFile,ch);
		int d;
		d = stoi(ch,0,16);
		if((d & 1<<7) !=0){
            d-=256;
        }
		P.loadDarr(d,i);
		i++;
	}
	
	DFile.close();
	
	

	ifstream IFile("ICache.txt");										//creating an array from icache
	i=0;
	while(i<256){
		getline(IFile,ch);
		int inst;
		inst = stoi(ch,0,16);
		P.loadInvec(inst,i);
		i++;
	}
	
	IFile.close();
	
	
	
	
    ifstream RFile("RF.txt");
    i=0;
    while(i<16){
		getline(RFile,ch);
		int r;
		r = stoi(ch,0,16);		
		if((r & 1<<7)!=0){
            r-=256;
        }
        P.loadReg(r,i);
        i++;
	}
	
	RFile.close();
	
	
	
	
	P.setBranch();
	while(1){															
        P.WB();
        P.MEM();
        P.EX();
        P.ID();
        
        if(P.controlHz()) continue;
        if(P.dataHz()) continue;
        
        else{
        P.IF();
		t = t + 1;
		if(P.checkEnd()) break;
		}
	}
	
	
	ofstream fileD;
	fileD.open("DCache0.txt");											
	i = 0;
	while(i<256){
		int x = P.getDarr(i);
        x = x & 255;
         pair<char,char>pi;
        if(x%16 >= 10){
            pi.second = ('a' + (x%16)-10); 
        }
        else{
            pi.second = ('0'+ x%16) ;    
        }
		x = x/16;
		 if(x%16 >= 10){

            pi.first = ('a' + (x%16)-10);
        }
        else{
            pi.first = ('0'+x%16);
        }
        fileD << pi.first << pi.second <<endl;
        i++;
	}
	
    fileD.close();
    
    
    ofstream fileRF;
    fileRF.open("RF0.txt");
    i = 0;
    while(i < 16){
		int x = P.getReg(i);
        x = x & 255;
        pair<char,char>pi;
        if(x%16 >= 10){
            pi.second = ('a' + (x%16)-10);
        }
        else{
            pi.second = ('0'+x%16);
        }
        x = x/16;
        if(x%16 >=10){
            pi.first = ('a' + (x%16)-10);
        }
        else{
             pi.first = ('0'+x%16);    
        }
        fileRF << pi.first << pi.second << endl;
        i++;
	}
    
    fileRF.close();
    ofstream file;
    file.open("Output.txt");
    file << "Total number of instructions executed\t: "<< incount << "\n";
    file << "Number of instructions in each class\n";
    file << "Arithmetic instructions\t\t\t\t: "<< arithinst << "\n";
    file << "Logical instructions\t\t\t\t: "<<logicinst << "\n";
    file << "Data instructions\t\t\t\t\t: "<<dintcount << "\n";
    file << "Control instructions\t\t\t\t: "<< ctincount << "\n";
    file << "Halt instructions\t\t\t\t\t: "<< haltcount << "\n";
	file << "Cycles Per Instruction\t\t\t\t: "<< (double)t/incount<< "\n";
    file << "Total number of stalls\t\t\t\t: "<<tot_stall << "\n";
    file << "Data stalls (RAW)\t\t\t\t\t: "<< dstall << "\n";
    file << "Control stalls\t\t\t\t\t\t: " << cstall << "\n";
    file.close();
}
