Team Name :  Akshay Bhasin (bhasia@rpi.edu) and  Rahul Prajapati (prajar@rpi.edu)

The read function tests if the file exists. If it doesnt it sends an error packet of value 1.
Then, we send the packets in order. If a reply is not received, we continue sending the packets. Everytime a packet is sent, we increment an attempt variable, which is incremented after 1 sec. Once it reaches 10, we terminate the connection prematurely since we have not received a response in 10 seconds. We have checked the correct transfer of single file, multiple files at the same time in octet mode. To check for correct transfer, we computed the md5sum of the files respectively.

The write function first test if file which is to be written exist. If it exist then it send appropriate error message. Other wise it sends the acknowledgement to the the server. When it receives data packets, it first check if it is the correct one. If it is not the correct data packet then it sends the acknowledgement asking for the data packet it wasnts. If the data packet received is correct then it send the acknowledgement for the data packet received. With every data packets received, it checks whether size of entire packet is 516. If it is less then that is the last packet to be received, thus the program after sending acknowledgement close the socket connection. Also the function provide timeout of 10 seconds and total retry of 5. It close the socket if packet is not received in given 10 seconds and after trying for 5 times.

 
