# IMS-pulse-detection
created C++ algorithm based on ims algorithm('Incremental Merge Segmentation'). Extra filterings and steps are included to modify the data values in the dataset provided.

**ims_v1.cpp** and ims_v1.h 
are the most recent file. Simply call dataToBuffer() function to put in raw data from ppg sensor(format should be double), and it will automatically show the bpm.
the function should be called once per 10ms (frequency = 100hz).

Initial BPM value will be shawn after getting values for 20 seconds, and then the bpm will be updated every 1 sec.


reference:<br />
https://ppg-beats.readthedocs.io/en/stable/functions/ims_beat_detector/ <br />
https://ieeexplore.ieee.org/document/6346628
