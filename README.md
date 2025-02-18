# IMS-pulse-detection
created C++ algorithm based on ims algorithm('Incremental Merge Segmentation'). Extra filterings and steps are included to modify the data values in the dataset provided.

#ims_v1.cpp is the most recent file. Simply call dataToBuffer() function to input data from ppg sensor(format should be double), and it will automatically show the bpm.


reference:<br />
https://ppg-beats.readthedocs.io/en/stable/functions/ims_beat_detector/ <br />
https://ieeexplore.ieee.org/document/6346628
