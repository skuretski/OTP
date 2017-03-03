# OTP

This is a basic C implementation of a one-time-pad encryption and 
decryption send/receive using multiple threads and daemons. 

Usage: 
$ otp_enc_d [port number] & 

$ otp_dec_d [port number] & 

$ keygen [length] > [destination key file] 

$ otp_enc [plain text file] [key file] [port number] > [destination text file] 

