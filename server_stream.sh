#!/bin/bash

# Nome del file video
FILE="video.mp4"

# Porta e indirizzo IP per lo streaming
PORT=1234
ADDRESS="0.0.0.0"

# Comando per avviare lo streaming con VLC
cvlc -vvv "$FILE" --sout "#rtp{sdp=rtsp://$ADDRESS:$PORT/stream}" --loop
