# Protocol

!!! The protocol is subject to be extended and/or changed until a stable v1.0 version of the firmware would be available !!!

For communication with the Raspberry PI Pico a simple human readable text protocol and message framing is used. Each command message starts with an '+' and ends with \<CR\>'. For each command a reply is send consisting of one or more messages. Each reply message starts with one of the following characters and ends with \<CR\>':

- '=' (command successful - single message reply)
- '?' (command not successful - followed by error)
- '-' (command successful - multi message reply)
- '.' (end message in case of multi message reply)
- '!' (push message)

The protocol is not strictly command->reply based as the command station might send push messages at any time. But it is guaranteed that replies are send in command order and that push messages are not send in between multi message replies.

### Currently the following commands are supported:

   ***
#### h

    Returns a multi message help reply.

   ***
#### cl [t|f]
    
    Returns or sets the value for the internal Pico led where
    - t sets the led on is the command staion is enabled
    - f sets the led off.

   ***
#### ct

    Returns the value of the internal temperature sensor.

   ***
#### cs [\<bits\>]

    Returns or sets the number of DCC sync bits \<bits\>.

   ***
#### ce [t|f]

    Returns or sets the command station output where
    - t enables the DCC output of the commend station and
    - f disables the DCC output.

   ***
#### cr

    Returns the content of the internal refresh buffer (for debugging purposes only).

   ***
#### cd \<addr\>

    Deletes the loco with address \<addr\> from the internal refresh buffer (for debugging purposes only).

   ***
#### ld \<addr\> [t|f|~]

    Returns or sets the direction of loco with address \<addr\> where
    - t is forward
    - f is backward and
    - ~ toggles the direction.

   ***
#### ls \<addr\> [<speed128]

    Return or sets the speed of loco with address \<addr\> where
    - speed 0 is stop
    - speed 1 is emergency stop and
    - speed 2 - 128 are the 126 speed steps supported by DCC.

   ***
#### lf \<addr\> \<no\> [t|f|~]

    Returns or sets the function \<no\> of loco with address c where
    - t sets the funtion on
    - f sets the function off and
    - ~ toggles the function.

   ***
#### lcvbyte \<addr\> \<idx\> \<cv\>

    Sets the byte value of the CV variable \<idx\> of loco with address \<addr\> to \<cv\>.

   ***
#### lcvbit \<addr\> \<idx\> \<bit\> t|f

    Sets the bit \<bit\> of the CV variable \<idx\> of loco with address \<addr\> where
    - t is on and
    - f is off.

   ***
#### lcv29bit5 \<addr\> t|f

    Sets the bit 5 of CV 29 of loco with address \<addr\> where
    - t is on and
    - f is off.

   ***
#### lladdr \<addr\> \<laddr\>

    Sets the long address \<laddr\> of loco with address \<addr\>.

   ***
#### lcv1718 \<addr\>

    Returns the value ov CV 17 and CV 18 of loco with address \<addr\>.


### Examples

The following examples demonstrate the usage of the text protocol. For each example the first block shows the command message (please remember to start each commend with '+') and the second block shows the reply.

Each command needs to be ended with a \<CR\> and each reply message does end with a \<CR\> as well. For better readability the \<CR\> is not shown.

- read temperature
    ```
    +ct
    ```

    ```
    =23.861501
    ```

- enable command station output
    ```
    +ce t
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
    