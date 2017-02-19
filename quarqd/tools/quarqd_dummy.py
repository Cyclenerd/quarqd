#!/python

import time
import socket

s=socket.socket()
s.bind(('localhost',8168))
s.listen(10)
sock, who = s.accept()

watts=0
cadence=90.0

while 1:
    print watts
    sock.send("<Power id='17p' timestamp='%f' watts='%d' />\n"%(time.time(),watts))
    time.sleep(60.0 / cadence)
    watts=watts+1
