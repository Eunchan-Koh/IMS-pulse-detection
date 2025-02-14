#include "matplotlibcpp.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<algorithm>


using namespace std;

namespace plt = matplotlibcpp;



int main(){
    vector<double> sig;
    string line;
    bool firstCheck = false;
    ifstream file("ourData.txt"); // example.txt
	if(file.is_open()){
		while(getline(file, line)) {
			// cout << "line is: " + line << endl;
			if(!firstCheck){
				firstCheck = true;
			}else{
				sig.push_back(stod(line));
			}

		}
		file.close(); //
	} else {
		cout << "Unable to open file";
		return 1;
	}

    //change frequency value to our value later
    int frequency = 100;

    int MAX_HEARTRATE = 220;//actually it is 220-your age. but use this constant instead

    //1/frequency for number of data per second, and 60/maxheartrate for maximum possible peak distance
    int min_freq_req = 60*frequency/MAX_HEARTRATE;//the length between each peak must be larger (not larger than equal to, but larger) than this!

    cout << "min_freq_req value is: " << min_freq_req << endl;

    int RECORD_MINUTE = sig.size()/frequency/60;
    cout << "record minute length is: " << RECORD_MINUTE << endl;
    cout << "signal size is: " << sig.size() << endl;

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
        }else{//yet this segment
            tempTotal += sig[i];
            segCount++;
        }
    }
    prevAvgSize = avgs.size();
    cout << "prevAvgSize is: " << prevAvgSize << endl;


    double totalAvg = 0;
    for(int i = 0; i < avgs.size(); i++){
        totalAvg += avgs[i];
    }
    totalAvg = totalAvg/avgs.size();
    cout << "total average = " << totalAvg << endl;
    cout << "total average check = " << totalAvg*1.25 << ", " << totalAvg*0.75 << endl;

    for(int i = 0; i < avgs.size(); i++){
        if(avgs[i] > totalAvg*1.25 || avgs[i] < totalAvg*0.75){
            avgs.erase(avgs.begin()+i);
            i-=1;//since the value is deleted, next value is pulled up to i again, so check ith value again
        }
    }
    cout << "now avgSize is: " << avgs.size() << endl;
    
    ////////////////////////////////
    //change(and ceil) sig values
    for(int i = 0; i < sig.size(); i++){
        sig[i] = (sig[i]/totalAvg)*(-1)+2;
        // sig[i] = ceil(sig[i]*1000)/100;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //signal modification ends
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //ims algorithm starts
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //segmentation
    //linez are divided into x1 and y1
    vector<double> x1;
    vector<double> y1;
    int segmentSize = 10;
    for(int i = 0; i < sig.size()/segmentSize; i++){
        x1.push_back(i*segmentSize);
        y1.push_back(sig[(i*segmentSize)]);
    }
    //get slopes of each segment
    vector<int> slope1;//only has -1, 0 or 1 values inside
    vector<double> slope2;//contains actual slope value after merging segments
    vector<double> amp;//has actual amplitude value of segments
    vector<double> duration;//duration of each segment
    double MIN_DURATION = 0.03*frequency;
    for(int i = 0; i < x1.size()-1; i++){
        int slopeSign = 0;
        if(y1[i+1]-y1[i] > 0){
            slopeSign = 1;
        }else if(y1[i+1]-y1[i] < 0){
            slopeSign = -1;
        }else{
            slopeSign = 0;
        }
        slope1.push_back(slopeSign);
    }
    //merging same sign
    for(int i = 0; i < x1.size()-1; i++){
        if(slope1[i] == slope1[i+1]){//next segment has same slope sign
            x1.erase(x1.begin()+i+1);
            y1.erase(y1.begin()+i+1);
            slope1.erase(slope1.begin()+i);
            i--;
        }
    }
    //get actual slopes, amplitude and duration of each segment
    for(int i = 0; i < x1.size()-1; i++){
        double tempSlope = (y1[i+1]-y1[i])/(x1[i+1]-x1[i]);
        slope2.push_back(tempSlope);
        amp.push_back(y1[i+1]-y1[i]);
        duration.push_back(x1[i+1]-x1[i]);
    }
    //if duration is too short, erase those segments
    //instead of erasing, add a 1 value to show that this value needs to be ignored.
    vector<bool> IgnoreVal;//if IgnoreVal[i] = true, ignore slope at i, which is slope between x1[i] and x1[i+1].
    for(int i = 0; i < x1.size()-1; i++){
        if(duration[i] < MIN_DURATION){
            cout << "is it even being called ever once" << endl;
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
    vector<double> sortingVec;
    //collect only positive amps
    for(int i = 0; i < amp.size(); i++){
        if(amp[i]>0) sortingVec.push_back(amp[i]);
        if(sortingVec.size() > 20) break;
    }


    //fast_low, fast_high, slow_low, slow_high
    vector<double> a = {0.5, 1.6, 0.6, 2.0};
    
    //getting ThAlow and ThAhigh
    double ThAlow = 0;
    double ThAhigh = 0;
    //between 0~1, base value was 0.95.
    double PRC_POSITION = 0.65;
    sort(sortingVec.begin(), sortingVec.end());
    double tempVal2 = sortingVec.size()*PRC_POSITION;
    double tempVal3 = ceil(tempVal2) - tempVal2;
    int tempVal4 = tempVal2;
    if(tempVal3 != 0){
        double tempVal = sortingVec[tempVal4+1]*(1-tempVal3) + sortingVec[tempVal4]*tempVal3;
        ThAlow = tempVal * 0.6;
        ThAhigh = tempVal * 1.8;
    }

    cout << "ThAlow and ThAhigh are: " << ThAlow << ", " << ThAhigh << endl;
    vector<double> peaks;
    vector<double> peaks_y;
    vector<double> onsets;

    int checkedFlag = 0;
    bool FOUND_L1 = false;
    vector<double> tempAmp;

    for(int i = 1; i < x1.size()-1; i++){
        if(FOUND_L1){
            if(!IgnoreVal[i]){
            if(slope2[i] > 0 && slope2[i-1] != 0 && slope2[i+1] != 0){//before&after slope must be non-zero, and this slope must be positive.
                // cout << "slope is: " << slope2[i] << endl;
                if(amp[i]>= ThAlow && amp[i] <= ThAhigh && x1[i+1] >= peaks[peaks.size()-1] + min_freq_req){//in rage, check for peak
                    ThAlow = (ThAlow + amp[i]*a[2])/2;
                    ThAhigh = amp[i] * a[3];
                    checkedFlag = 0;
                    tempAmp.push_back(amp[i]);
                    peaks.push_back(x1[i+1]);
                    peaks_y.push_back(y1[i+1]);
                    onsets.push_back(x1[i]);
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
                onsets.push_back(x1[i]);
                peaks.push_back(x1[i+1]);
                peaks_y.push_back(y1[i+1]);
                tempAmp.push_back(amp[i]);
            }
            }
        }
    }
    // cout << ThAhigh << ", " << ThAlow << endl;

    //currently slope1(slope signs), slopes2(actual slope), amplitude and durations uses ith point and i+1th point for comparing - ex) if duration[i] is called, it is duration between points at x1[i] and x1[i+1]
    
    cout << "number of peaks found is: " << peaks.size() << endl;

    // cout << "pulse bpm is: " << y.size()/RECORD_MINUTE << endl;
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //plotting
    plt::plot(sig);
    // std::vector<double> x = {0};//for x axis of where peak is found
    // std::vector<double> y = {1.5};//peak output value found, in order of x axis order(from left)
    // plt::plot(x, y, "or");

    plt::plot(x1,y1, "-");
    plt::plot(peaks, peaks_y, "or");
    plt::title("Testing values");
    plt::show();

}