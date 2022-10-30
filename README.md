# pico-cs

[![REUSE status](https://api.reuse.software/badge/github.com/stfnmllr/pico-cs)](https://api.reuse.software/info/github.com/stfnmllr/pico-cs)

pico-cs is a proof-of-concept for a model railway command station talking DCC (Digital Command Control) defined by the [NMRA DCC working group](https://www.nmra.org/dcc-working-group) and using the [Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/) as DCC signal generator.

pico-cs is intended for skilled users with expert levels of model railway electronics and protocol knowledge.

## Why Raspberry Pi Pico

- Its support of programmable IO (PIO) enables implementing the core DCC protocol within a few lines of assembler code
- Due to its dual cores the DCC signal generation and the command interface are implemented using different cores being run in parallel
- Its small form factor and cost effectiveness allows and supports multiple command stations to be used as part of the model railroad layout
- Its great ducumentation 
- And last but not least the fun using it

## Hardware

- Raspberry Pi Pico / Pico H
- A PC, Laptop, Raspberry Pi or any suitable device with an USB interface to flash the firmware
- A model railway booster
- A model railroad locomotive roller test stand
- A DCC decoder equiped test locomotive

### Booster

As there is a lot of booster alternatives (motor shield, H-bridge, commercial booster, ...) the selection and connection options to the Pico goes beyond the scope of this document. You might find potential solutions searching the internet. Worth to know
- Pico DCC signal output is on GP2 with 3.3V level
- As most digital booster inputs would not work with 3.3V (please consult booster documentation) you need to choose a safe and reliable solution for level conversion

## Firmware

Please see [firmware folder](https://github.com/stfnmllr/pico-cs/tree/main/firmware) for information about flashing the pico-cs firmware and the implemented protocol.

## Features

- Pico firmware implementing DCC comands to control model railway locomotives
- Simple command human readable and debug friendly text protocol
  - which can be used directly via serial terminal programs supporting serial over USB
  - or easily integrated into any programming language or tool supporting serial over USB communication

## Outlook

- Pico W support
- [Go](https://go.dev/) client library 

## Licensing

Copyright 2021-2022 Stefan Miller and pico-cs contributers. Please see our [LICENSE](LICENSE) for copyright and license information. Detailed information including third-party components and their licensing/copyright information is available [via the REUSE tool](https://api.reuse.software/info/github.com/stfnmllr/pico-cs).
