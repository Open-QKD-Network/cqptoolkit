# SSH Tunnel Two Node Cqptoolkit Setup on AWS

Note: Only select ports on AWS servers are open. Use the ports in this demo, or select use ports 8000 or 9000-9003.

## Steps to Reproduce
On each AWS server
- Navigate to `build-cqptoolkit`
- Create a site agent configuration of the form (using private AWS IP):
    ```json
    {
        "name": "SiteA",
        "id": "9893e345-d3b0-4a98-a6d2-4663190e3db9",
        "bindAddress":"172.31.28.5",
        "connectionAddress":"172.31.28.5",
        "listenPort": 9002,
        "credentials": {}
    }
    ```
-Run the site agent with `./src/Tools/SiteAgentRunner/SiteAgentRunner -c siteagent-ID.json`

On each local machine (connect to different AWS server on each one)
- Make the ssh tunnel: `ssh -R 9001:192.168.2.22:9001 -N -f -i Basquana_1.pem ubuntu@3.84.128.28`. You can verify this worked on the server by running: `sudo netstat -tulpn` and looking for the port `9001`. To kill running ssh tunnels, `kill` the associated PID given by `netstat`.
- Navigate to `build-cqptoolkit`
- Create a dummy driver configuration of the form (using public AWS IP):
    ```json
    {
        "controlParams": {
            "config": {
                "id": "dummyqkd_a_b_a",
                "side": "Alice",
                "kind": "dummyqkd"
            },
            "bindAddress": "192.168.2.22:9001",
            "controlAddress": "3.84.128.28:9001",
            "siteAgentAddress": "3.84.128.28:9002"
        }
    }
    ```
- Run the dummy drivew with `./src/Drivers/DummyQKDDriver/DummyQKDDriver -c dummy-ID.json`

Now join site agents from any device using their public IPs: `./src/Tools/SiteAgentCtl/SiteAgentCtl -c 3.84.128.28:9002 -j 54.159.53.158:9002` 

Now from an AWS server we can request a key using the private IPs: `./src/Tools/SiteAgentCtl/SiteAgentCtl -c 172.31.28.5:9002 -k 172.31.20.54:9002`
