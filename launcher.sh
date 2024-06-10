#!/bin/bash


N_CLIENTS=10

run_client() {
    local username="lab"
    local password="$1"
    {
        echo "$username"  
        sleep 2
        echo "$password"  
        sleep 2
        echo "movie"    
        sleep 2
    } | ./client
}


for ((i=1; i<=N_CLIENTS; i++)); do
    run_client $i &
done


wait
echo "All clients have finished."
