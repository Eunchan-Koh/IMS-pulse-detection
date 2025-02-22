
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include<algorithm>


using namespace std;


//change frequency value to our value later if it is not correct.
int frequency = 100;
int MAX_HEARTRATE = 220;//actually it is 220-your age. but use this constant instead

//1/frequency for number of data per second, and 60/maxheartrate for maximum possible peak distance
int min_freq_req = 60*frequency/MAX_HEARTRATE;//the length between each peak must be larger (not larger than equal to, but larger) than this!

vector<double> sig;
vector<double> buffer_sig;

void dataToBuffer(double data);
double getTotalAvg(vector<double> sig);
vector<double> sigInversion(vector<double> sig);
void segmentation();
void imsAlgorithm();
void PulseCalculation();

double totalAvg = 0;

//linez are divided into X_POS and Y_POS. segments' each point's values.
vector<double> SEG_X_POS;
vector<double> SEG_Y_POS;

//previous end of dataset - segment count value.
int PREV_END_OF_DATA = 0;
int PREV_SLOPE_INDEX = 0;

//get actual slopes, amplitude and duration of each segment
vector<double> slope2;//contains actual slope value after merging segments
vector<double> amp;//has actual amplitude value of segments
vector<double> duration;//duration of each segment

//if duration is too short, erase those segments
//instead of erasing, add a 1 value to show that this value needs to be ignored.
double MIN_DURATION = 0.03*frequency;
vector<bool> IgnoreVal;//if IgnoreVal[i] = true, ignore slope at i, which is slope between x1[i] and x1[i+1].

//collect only positive amps
vector<double> sortingVec;

//between 0~1, base value was 0.95.
double PRC_POSITION = 0.65;
   

//fast_low, fast_high, slow_low, slow_high
vector<double> a = {0.5, 1.6, 0.6, 2.0};

//getting ThAlow and ThAhigh
double ThAlow = 0;
double ThAhigh = 0;

int checkedFlag = 0;
bool FOUND_L1 = false;
vector<double> tempAmp;

//peak values
vector<double> peaks;
vector<double> peaks_y;
vector<double> onsets;

//for pulse calculation
bool firstPulseCalculation = true;
int bpm = 0;
double timeSeg = 1000/frequency;//change into ms unit. basically, 10ms. x unit 1 block has this much value.

int main(){
    string line;
    bool firstCheck = false;
    ifstream file("ourData2.txt"); // example.txt
	if(file.is_open()){
		while(getline(file, line)) {
            stringstream temas(line);
            string firstNum;
            string secondNum;
            double secondNumber;
            if(getline(temas, firstNum, ',')){
                // cout << "line is: " + firstNum << endl;
            }
            if(getline(temas, secondNum, ',')){
                
                secondNumber = stod(secondNum);
                // cout << "line is: " << secondNumber << endl;
            }
			
			if(!firstCheck){
				firstCheck = true;
			}else{
                if(!secondNum.empty()){
                    // sig.push_back(secondNumber);
                    dataToBuffer(stod(secondNum));
                }
				    
                // cout << "working yet..." << endl;
                //temporaryily close
                // dataToBuffer(stod(line));
                // dataToBuffer(stod(secondNum));
			}

		}
		file.close(); //
	} else {
		cout << "Unable to open file";
		return 1;
	}

}

double getTotalAvg(vector<double> sig){
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //signal modification
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int prevAvgSize = 0;
    int segSize = 30;
    int segCount = 0;
    vector<double> avgs;
    double tempTotal = 0;
    for(int i = 0; i < sig.size(); i++){
        if(segCount >= segSize){//next segment
            avgs.push_back(tempTotal/segSize);
            tempTotal = 0;
            segCount = 0;
            tempTotal += sig[i];
            segCount++;
        }else{//yet this segment
            tempTotal += sig[i];
            segCount++;
        }
    }
    prevAvgSize = avgs.size();
    // cout << "prevAvgSize is: " << prevAvgSize << endl;


    double totalAvg = 0;
    for(int i = 0; i < avgs.size(); i++){
        totalAvg += avgs[i];
    }
    totalAvg = totalAvg/avgs.size();
    // cout << "total average = " << totalAvg << endl;
    // cout << "total average check = " << totalAvg*1.25 << ", " << totalAvg*0.75 << endl;

    for(int i = 0; i < avgs.size(); i++){
        if(avgs[i] > totalAvg*1.25 || avgs[i] < totalAvg*0.75){
            avgs.erase(avgs.begin()+i);
            i-=1;//since the value is deleted, next value is pulled up to i again, so check ith value again
        }
    }
    // cout << "now avgSize is: " << avgs.size() << endl;

    return totalAvg;
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //signal modification ends
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

int inverseBound = 0;
vector<double> sigInversion(vector<double> sig){
    ////////////////////////////////
    //change(and ceil) sig values
    for(int i = inverseBound; i < sig.size(); i++){
        // sig[i] = (sig[i]/totalAvg)*(-1)+2;
        sig[i] = (sig[i])*(-1);
        // sig[i] = ceil(sig[i]*1000)/100;
    }
    inverseBound = sig.size();
    return sig;
}

int firstSegCount = 0;
vector<int> slope1;//only has -1, 0 or 1 values inside
void segmentation(){
    sig = sigInversion(sig);
    int segmentSize = 10;
    // cout << "sig size cal is: " << sig.size()/segmentSize << endl;
    for(int i = firstSegCount; i < sig.size()/segmentSize; i++){
        SEG_X_POS.push_back(i*segmentSize);
        SEG_Y_POS.push_back(sig[(i*segmentSize)]);
    }
    firstSegCount = sig.size()/segmentSize;
    
    // cout << "PREV_END_OF_DATA value is: " << PREV_END_OF_DATA << endl;

    // cout << "last value is: " << SEG_X_POS[SEG_X_POS.size()-1] << endl;
    
    ///////////////////////////////////////////////////////////////////////////
    //up to this point, working fine. but segment merging is not working properly, maybe ignored due to error. Check.
    
    
    
    //get slopes of each segment
    
    //check x = 2680 point
    //error on this portion - maybe adding extra slope or wrong slope
    for(int i = PREV_END_OF_DATA; i < SEG_X_POS.size()-1; i++){
        int slopeSign = 0;
        if(SEG_Y_POS[i+1]-SEG_Y_POS[i] > 0){
            slopeSign = 1;
        }else if(SEG_Y_POS[i+1]-SEG_Y_POS[i] < 0){
            slopeSign = -1;
        }else{
            slopeSign = 0;
        }
        if(i >= slope1.size())
            slope1.push_back(slopeSign);
    }
    if(slope1.size() != SEG_X_POS.size()-1){
        cout << "error! size error!" << endl;
    }
    // if(SEG_X_POS.size() > 2000){
    //     cout << "slope at 2000: " << slope1[2000] << endl;
    // }
    // cout << "!!!size check: " << SEG_X_POS.size() << ", " << slope1.size() << ", " << slope1[608] << ", " << slope1[609] << ", " << SEG_X_POS[610]  << endl;
    //merging same sign
    // cout << "!!!!!!!prev is: " << PREV_END_OF_DATA << ", and slope1 size is: " << slope1.size() << endl;
    for(int i = 0; i < slope1.size()-1; i++){
        // if(i > slope1.size()){cout << "out of range!!" << endl;}
        // cout << "is it being called?" << endl;
        if(slope1[i] == slope1[i+1]){//next segment has same slope sign
            SEG_X_POS.erase(SEG_X_POS.begin()+i+1);
            SEG_Y_POS.erase(SEG_Y_POS.begin()+i+1);
            slope1.erase(slope1.begin()+i);
            i--;
        }
    }
    // cout << "!!!!!!!!!after is: " << PREV_END_OF_DATA << ", and slope1 size is: " << slope1.size() << endl;
    //slope size at the end should be 269 with our sample
    // PREV_SLOPE_INDEX = slope1.size()-1;
    PREV_END_OF_DATA = SEG_X_POS.size()-1;
    imsAlgorithm();
}

int bufferSize = 100;
void dataToBuffer(double data){
    buffer_sig.push_back(data);
    if(sig.empty()){
        if(buffer_sig.size() >= 2000){
            for(int i = 0; i < buffer_sig.size(); i++){
                sig.push_back(buffer_sig[i]);
            }
            // sig.insert(sig.end(), buffer_sig.begin(), buffer_sig.end()-1);
            buffer_sig.clear();
            segmentation();
        }
    }else{
        if(buffer_sig.size() >= bufferSize){
            for(int i = 0; i < buffer_sig.size(); i++){
                sig.push_back(buffer_sig[i]);
            }
            // sig.insert(sig.end(), buffer_sig.begin(), buffer_sig.end()-1);
            buffer_sig.clear();
            segmentation();
        }
    }
    
}

void imsAlgorithm(){
    //get actual slopes, amplitude and duration of each segment
    int tempSizeIndex = duration.size();
    for(int i = tempSizeIndex; i < SEG_X_POS.size()-1; i++){
        double tempSlope = (SEG_Y_POS[i+1]-SEG_Y_POS[i])/(SEG_X_POS[i+1]-SEG_X_POS[i]);
        slope2.push_back(tempSlope);
        amp.push_back(SEG_Y_POS[i+1]-SEG_Y_POS[i]);
        duration.push_back(SEG_X_POS[i+1]-SEG_X_POS[i]);
    }
    //if duration is too short, erase those segments
    
    for(int i = tempSizeIndex; i < duration.size(); i++){
        if(duration[i] < MIN_DURATION){
            // cout << "it is not expected to be called in this code" << endl;
            IgnoreVal.push_back(true);
            // slope2.erase(slope2.begin()+i);
            // amp.erase(amp.begin()+i);
            // duration.erase(duration.begin()+i);
            // i--;
        }else{
            IgnoreVal.push_back(false);
        }
    }
    
    
    //create artifs, clipp if needed, but not created yet
    
    //collect only positive amps
    for(int i = tempSizeIndex; i < amp.size(); i++){
        if(amp[i]>0) sortingVec.push_back(amp[i]);
        if(sortingVec.size() > 20) break;
    }


    
    if(ThAlow == 0 && ThAhigh == 0){
        sort(sortingVec.begin(), sortingVec.end());
        double tempVal2 = sortingVec.size()*PRC_POSITION;
        double tempVal3 = ceil(tempVal2) - tempVal2;
        int tempVal4 = tempVal2;
        if(tempVal3 != 0){
            double tempVal = sortingVec[tempVal4+1]*(1-tempVal3) + sortingVec[tempVal4]*tempVal3;
            ThAlow = tempVal * 0.6;
            ThAhigh = tempVal * 1.8;
        }
    }
    

    // cout << "ThAlow and ThAhigh are: " << ThAlow << ", " << ThAhigh << endl;
    

    

    for(int i = 1; i < slope2.size()-1; i++){
        if(FOUND_L1){
            if(!IgnoreVal[i]){
            if(slope2[i] > 0 && slope2[i-1] != 0 && slope2[i+1] != 0){//before&after slope must be non-zero, and this slope must be positive.
                // cout << "slope is: " << slope2[i] << endl;
                if(amp[i]>= ThAlow && amp[i] <= ThAhigh && SEG_X_POS[i+1] >= peaks[peaks.size()-1] + min_freq_req){//in rage, check for peak
                    ThAlow = (ThAlow + amp[i]*a[2])/2;
                    ThAhigh = amp[i] * a[3];
                    checkedFlag = 0;
                    tempAmp.push_back(amp[i]);
                    peaks.push_back(SEG_X_POS[i+1]);
                    peaks_y.push_back(SEG_Y_POS[i+1]);
                    onsets.push_back(SEG_X_POS[i]);
                }else{//outside of range of ThAlow and ThAhigh
                    if(checkedFlag > 0){
                        if(tempAmp.size() > 4){//more than 4 items in tempAmp
                            ThAlow = (ThAlow + *min_element(tempAmp.end()-4, tempAmp.end())*a[0])/2;
                            ThAhigh = *max_element(tempAmp.end()-4, tempAmp.end()) * a[1];
                        }else{//less or equal
                            ThAlow = (ThAlow + *min_element(tempAmp.begin(), tempAmp.end())*a[0])/2;
                            ThAhigh = *max_element(tempAmp.begin(), tempAmp.end()) * a[1];
                        }
                    }
                    checkedFlag++;
                    //add values on artif since slope was positive but out of target range.
                }
            }
            // else if
            }
        }else{
            if(!IgnoreVal[i]){
            if(slope2[i] > 0 && duration[i] >= MIN_DURATION && amp[i]>=ThAlow && amp[i] <= ThAhigh){//first proper line found!
                // cout << "first peak is found at: " << x1[i] << "!" << endl;
                FOUND_L1 = true;
                ThAlow = amp[i] * 0.5;
                ThAhigh = amp[i] * 2.0;
                onsets.push_back(SEG_X_POS[i]);
                peaks.push_back(SEG_X_POS[i+1]);
                peaks_y.push_back(SEG_Y_POS[i+1]);
                tempAmp.push_back(amp[i]);
            }
            }
        }
    }

    PulseCalculation();
}

double bpmDuration = 0;
void PulseCalculation(){
    if(firstPulseCalculation){
        int tempTimeTotal = 0;
        for(int i = 0; i < peaks.size()-1; i++){
            tempTimeTotal += peaks[i+1] - peaks[i];
        }
        bpmDuration = tempTimeTotal/(peaks.size()-1);
        bpm = 60000/(timeSeg*bpmDuration);
        cout << "bpm is: " << bpm << endl;
        firstPulseCalculation = false;
    }else{
        if(peaks.size() > 10){
            int tempTimeTotal = 0;
            int pointCount = 0;
            //max 9 durations
            bool doublecheck = false;
            for(int i = peaks.size()-10; i < peaks.size()-1; i++){
                int timeDiff = peaks[i+1] - peaks[i];
                if(timeDiff > 1.5*bpmDuration){
                    if(doublecheck){
                        tempTimeTotal += timeDiff;
                        pointCount++;
                    }
                    doublecheck = true;
                }else{
                    tempTimeTotal += timeDiff;
                    pointCount++;
                }
            }
            bpmDuration = tempTimeTotal/pointCount;
            bpm = 60000/(timeSeg*bpmDuration);
            cout << "bpm is: " << bpm << endl;
        }
    }
    
}

void changeFrequency(int newVal){
    frequency = newVal;
}
