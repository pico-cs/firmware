# Protocol

!!! The protocol is subject to be extended and/or changed until a stable v1.0 version of the firmware would be available !!!

For communication with the Raspberry PI Pico a simple human readable text protocol and message framing is used. Each command message starts with an '+' and ends with \<CR\>'. For each command a reply is send consisting of one or more messages. Each reply message starts with one of the following characters and ends with \<CR\>':

- '=' (command successful - single message reply)
- '?' (command not successful - followed by error)
- '-' (command successful - multi message reply)
- '.' (end message in case of multi message reply)
- '!' (push message)

The protocol is not strictly command->reply based as the command station might send push messages at any time. But it is guaranteed that replies are send in command order and that push messages are not send in between multi message replies.

### Push messages

   ***
#### Info messages 

    Some of the push messages are info messages provided to the USB over serial connection only.

    Examples:

    !wifi: failed to connect
    !wifi: connecting...
    !wifi: connected
    !tcp: starting server host [...]

   ***
#### GPIO input event messages

    Event id: ioie

    For the free available GPIO inputs an event is raised in case the state of the GPIO has changed. The event returns the GPIO number and the value where 
    - t represents a high and
    - f a low level

    Between two events there can be more than one state changes for a single GPIO so that the value of the GPIO input might be equal to the value of the last event for the same GPIO.

    Example:

    !ioie: 5 t

    State change(s) of GPIO number 5 with a final high level value

### Currently the following commands are supported:

   ***
#### h

    Returns a multi message help reply.

   ***
#### b

    Returns board information:
    - 'pico' | 'pico_w' dependent on board
    - unique board id
    - mac address (pico_w only) 

   ***
#### s
    
    Stores the command station CV variables persistent on flash memory.

   ***
#### t

    Returns the value of the internal temperature sensor.

   ***
#### cv 0..6 [0..255]

    Returns or sets a command station CV variable via index.

    Currently the following CV indexes are supported:

    - 0: main track configuration flags:                                0..255 default:  1 (led enabled)
      - Switch led on when main track enabled:                          1
      - Switch BiDi cutout on for main track:                           2
      CV value is the sum of all flags to be enabled:
      e.g. enable Led and BiDi -> CV value: 1 + 2 = 3 
    - 1: number of DCC synchronization bits:                            17..32 default: 17
    - 2: number of DCC decoder command repetition:                       1..5  default:  1
    - 3: number of DCC CV command repetitions:                           2..5  default:  2
    - 4: number of DCC accessory decoder repetitions:                    1..5  default:  2
    - 5: BiDi (microseconds until power off after end bit):             26..32 default: 26
    - 6: BiDi (microseconds to power on before start of 5th sync bit):  10..16 default: 12

    All changes take immediate effect except the BiDi flag. This change takes effect when enabling the main track output the next time.

   ***
#### mte [t|f]

    Returns or sets the main track output where
    - t enables the output and
    - f disables the output.

   ***
#### ld \<addr\> [t|f|~]

    Returns or sets the direction of loco with address <addr> where
    - t is forward
    - f is backward and
    - ~ toggles the direction.

   ***
#### ls \<addr\> [0..127]

    Return or sets the speed of loco with address <addr> where
    - speed 0 is stop
    - speed 1 is emergency stop and
    - speed 2 - 127 are the 126 speed steps supported by DCC.
    
   ***
#### lf \<addr\> \<no\> [t|f|~]

    Returns or sets the function <no> of loco with address <addr> where
    - t sets the funtion on
    - f sets the function off and
    - ~ toggles the function.

   ***
#### lcvbyte \<addr\> \<idx\> 0..255>

    Sets the byte value of the CV variable <idx> of loco with address <addr>.

   ***
#### lcvbit \<addr\> \<idx\> 0..7 t|f

    Sets the bit 0..7 of the CV variable <idx> of loco with address <addr> where
    - t is on and
    - f is off.

   ***
#### lcv29bit5 \<addr\> t|f

    Sets the bit 5 of CV 29 of loco with address <addr> where
    - t is on and
    - f is off.

   ***
#### lladdr \<addr\> \<laddr\>

    Sets the long address <laddr> of loco with address <addr>.

   ***
#### lcv1718 \<addr\>

    Returns the value ov CV 17 and CV 18 of loco with address <addr>.

   ***
#### af \<addr\> 0|1 t|f

    Sets one of the paired outputs of a simple accessory decoder to active or inactive where
    - addr is the '11-bit' DCC accessory decoder address in the range of 0..4047 with 4044-4047 as NMRA broadcast address
    - 0|1 determines one of the paired outputs and
    - t|f activates respectively deactivates the output

   ***
#### at \<addr\> 0|1 0..127

    Sets the activation time for one of the paired outputs of a simple accessory decoder (if supported) where
    - addr is the '11-bit' DCC accessory decoder address in the range of 0..4047 with 4044-4047 as NMRA broadcast address
    - 0|1 determines one of the paired outputs and
    - 0..127 is the activation time in 100 millisecond steps with
      - 0 deactivating the output and
      - 127 activating the output permanently

   ***
#### as \<addr\> 0...255

    Sets the data byte for an extended accessory decoder like transmitting aspect control to a signal decoder where
    - addr is the '11-bit' DCC accessory decoder address in the range of 0..4047 with 4047 as broadcast address

   ***
#### ioadc 0..4

    Returns the 'raw' value (assume ADC_VREF == 3.3 V) of the ADC input with
    - 0: GPIO26
    - 1: GPIO27
    - 2: GPIO28
    - 3: GPIO29 (pico internal)
    - 4: pico temperature sensor

   ***
#### ioval \<gpio\> [t|f|~]

    Returns or sets the gpio value where
    - t is on and
    - f is off and
    - ~ toggles the value

    Allowed GPIO values are
    - 10..22 

   ***
#### iodir \<gpio\> [t|f|~]

    Returns or sets the gpio direction where
    - t is out and
    - f is in and
    - ~ toggles the direction

    Allowed GPIO values are
    - 10..22 

   ***
#### ioup \<gpio\> [t|f|~]

    Returns or sets the gpio pull-up where
    - t is on and
    - f is off and
    - ~ toggles the pull-up

    Allowed GPIO values are
    - 10..22 

   ***
#### iodown \<gpio\> [t|f|~]

    Returns or sets the gpio pull-down where
    - t is on and
    - f is off and
    - ~ toggles the pull-down

    Allowed GPIO values are
    - 10..22 

### Examples

The following examples demonstrate the usage of the text protocol. For each example the first block shows the command message (please remember to start each commend with '+') and the second block shows the reply.

Each command needs to be ended with a \<CR\> and each reply message does end with a \<CR\> as well. For better readability the \<CR\> is not shown.

- read temperature
    ```
    +t
    ```

    ```
    =23.861501
    ```

- enable command station main track output
    ```
    +mte t
    ```

    ```
    =t
    ```

- get loco speed
    ```
    +ls 3
    ```

    ```
    ?nodata
    ```
    Get loco speed of loco with address 3. As the loco is not yet part of the internal refresh buffer a 'nodata' error message is returned.

- set loco direction
    ```
    +ld 3 t
    ```

    ```
    =t
    ```
    The direction of loco with address 3 is set to forward. A success message with the forward value is returned.

    Do it again...
    ```
    +ld 3 t
    ```

    ```
    ?nochange
    ```
    As the loco direction of loco with address 3 was already set to forward a 'nochange' error message is retuned.
    
- toggle loco function 
    ```
    +lf 3 0 ~
    ```

    ```
    =t
    ```
    The function 0 (light) of loco with address 3 is toggled. A success message with the new value (here on) of the function is returned.
    