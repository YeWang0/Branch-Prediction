//Ye Wang
//CPEG652
//October 23,2015
#include "predictor.h"


#define PHT_CTR_MAX  3
#define PHT_CTR_INIT 2

#define HIST_LEN1   15
#define HIST_LEN2   14
#define HIST_LEN3   14

#define MAX_STATE 5
/////////////// STORAGE BUDGET JUSTIFICATION ////////////////
// Total storage budget: 16KB + 1024 bits
// Total PHT counters: 2^16
// Total PHT size = 2^15 * 2 bits/counter + 2^14 * 2 bits/counter * 2 = 2^17 bits = 16KB
// GHR size: 32 bits
// Prediction result counter size: 3 bits
// Voter size: 2 bits
// Total Size = PHTs size + GHR size + Prediction result counter size + Voter size
//                  =16KB+37 bits
/////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

PREDICTOR::PREDICTOR(void){
    //initialize
    ghr= 0;
    //prediction result recorder
    state=0;
    //PHTtable 1
    historyLength[1]    = HIST_LEN1;
    numPhtEntries[1]    = (1<< HIST_LEN1);
    pht[1] = new UINT32[numPhtEntries[1]];
    for(UINT32 ii=0; ii< numPhtEntries[1]; ii++){
        pht[1][ii]=PHT_CTR_INIT;
    }
    //PHT table 2
    historyLength[2]    = HIST_LEN2;
    numPhtEntries[2]    = (1<< HIST_LEN2);
    pht[2] = new UINT32[numPhtEntries[2]];
    for(UINT32 ii=0; ii< numPhtEntries[2]; ii++){
        pht[2][ii]=PHT_CTR_INIT;
    }
    //PHT table 3
    historyLength[3]    = HIST_LEN3;
    numPhtEntries[3]    = (1<< HIST_LEN3);
    pht[3] = new UINT32[numPhtEntries[3]];
    for(UINT32 ii=0; ii< numPhtEntries[3]; ii++){
        pht[3][ii]=PHT_CTR_INIT;
    }
    
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bool   PREDICTOR::GetPrediction(UINT32 PC){

    UINT32 phtIndex[4];
    UINT32 phtCounter[4];
    //predict by checking the specific PHT counters
    phtIndex[1]= (PC^ghr) % (numPhtEntries[1]);
    phtCounter[1]= pht[1][phtIndex[1]];
    
    phtIndex[2]= ((PC<<7)+(ghr&((1<<7)-1))) % (numPhtEntries[2]);
    phtCounter[2]= pht[2][phtIndex[2]];
    
    phtIndex[3]= (PC^(ghr>>8)) % (numPhtEntries[3]);
    phtCounter[3]= pht[3][phtIndex[3]];
    
    //vote to get the result of prediction
    int vote=0;
     if(phtCounter[1]> PHT_CTR_MAX/2){
         vote+=1;
     }else{
         vote-=1;
     }
    if(phtCounter[2]> PHT_CTR_MAX/2){
        vote+=1;
    }else{
        vote-=1;
    }
    if(phtCounter[3]> PHT_CTR_MAX/2){
        vote+=1;
    }else{
        vote-=1;
    }
//Change prediction when state(prediction result recorder) ==MAX_STATE
    if(vote>0){
        if (state==MAX_STATE) {
            state-=1;
            return NOT_TAKEN;
        }else
            return TAKEN;
    }else{
        return NOT_TAKEN;
    }
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void  PREDICTOR::UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget){
  
    UINT32 phtIndex[4];
    UINT32 phtCounter[4];
    
    phtIndex[1]= (PC^ghr) % (numPhtEntries[1]);
    phtCounter[1]= pht[1][phtIndex[1]];
    
    phtIndex[2]= ((PC<<7)+(ghr&((1<<7)-1))) % (numPhtEntries[2]);
    phtCounter[2]= pht[2][phtIndex[2]];
    
    phtIndex[3]= (PC^(ghr>>8)) % (numPhtEntries[3]);
    phtCounter[3]= pht[3][phtIndex[3]];

    int vote=0;
    if(phtCounter[1]> PHT_CTR_MAX/2){
        vote+=1;
    }else{
        vote-=1;
    }
    if(phtCounter[2]> PHT_CTR_MAX/2){
        vote+=1;
    }else{
        vote-=1;
    }
    if(phtCounter[3]> PHT_CTR_MAX/2){
        vote+=1;
    }else{
        vote-=1;
    }
    //  // update the state
    if (((vote<0)&&(resolveDir == TAKEN))||((vote>0)&&(resolveDir == NOT_TAKEN))) {
        if (state<MAX_STATE) {
            state++;
        }
    }
    else{
        if (state>0) {
            state--;
        }
    }
    //get the predict result
    bool select;
    if (vote>0) {
        select=TAKEN;
    }else
        select=NOT_TAKEN;
    
    //partial update the PHT
    if (select==resolveDir) {
        if(resolveDir==TAKEN){
            for (int i=1; i<4; i++) {
                if (pht[i][phtIndex[i]]>PHT_CTR_MAX/2) {
                    pht[i][phtIndex[i]] = SatIncrement(phtCounter[i], PHT_CTR_MAX);
                }
            }
        }else{
            for (int i=1; i<4; i++) {
                if (pht[i][phtIndex[i]]<=PHT_CTR_MAX/2) {
                    pht[i][phtIndex[i]] = SatDecrement(phtCounter[i]);
                }
            }
        }
    }
    else{
        if(resolveDir==TAKEN) {
            for (int i=1; i<4; i++) {
                pht[i][phtIndex[i]] = SatIncrement(phtCounter[i], PHT_CTR_MAX);
                }
        }else{
            for (int i=1; i<4; i++) {
                pht[i][phtIndex[i]] = SatDecrement(phtCounter[i]);
            }
        }
  }

  // update the GHR
  ghr = (ghr << 1);

  if(resolveDir == TAKEN){
    ghr++; 
  }

}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void    PREDICTOR::TrackOtherInst(UINT32 PC, OpType opType, UINT32 branchTarget){

  // This function is called for instructions which are not
  // conditional branches, just in case someone decides to design
  // a predictor that uses information from such instructions.
  // We expect most contestants to leave this function untouched.

  return;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
