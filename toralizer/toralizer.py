import os
import sys
import time
import socket
import subprocess
import logging
from stem.control import Controller
from stem import Signal
from stem.connection import PasswordAuthFailed, AuthenticationFailure

# Set up logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('toralizer')

class Toralizer:
    # Constructor - sets up the initial state of the object
    def __init__(self, socks_port=9050, control_port=9051, password=None):
        # Store the SOCKS port (where Tor's proxy listens)
        self.socks_port = socks_port
        # Store the control port (where we can send commands to Tor) 
        self.control_port = control_port
        # Store the password for the control port   
        self.password = password
        # We'll store the controller connection here once established
        self.controller = None
        
    def is_tor_running(self):
        # This method checks if Tor is already running by attempting to connect to the SOCKS port
        try:
            # new socket object for network connection
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # Set a timeout so we don't wait forever if the port isn't responding
            s.settimeout(3)
            # Try to connect to the Tor SOCKS port on localhost 
            s.connect(('127.0.0.1', self.socks_port))
            # If we get here, the connection was successful, so close the socket
            s.close()
            return True
        
        except (socket.error, socket.timeout):
            # If we get a socket error or timeout, it means we couldn't connect
            # This indicates Tor is not running or not accessible
            return False