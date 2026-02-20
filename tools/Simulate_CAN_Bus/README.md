1. Run `./setup.sh`
2. Run `source environment/bin/activate`

The above will get your environment set up with the right libraries.

## Usage
1. Change `can_messages.yaml` to send the CAN messages you want. `num_messages_in_burst` is the number of messages you will send with a **1 millisecond** delay. These bursts happen every `interval` milliseconds. `board_delay` is the time in milliseconds before that ID starts sending once you run the script. This is to imitate the startup behavior of the car so your tests actually match what happens on the car!
