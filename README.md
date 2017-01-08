# win-caffeinate
A port of `caffeinate` command of macOS to Windows. It can prevent the display and system from idle sleeping.

# Example
```shell
$ caffeinate make
```
`caffeinate` executes `make` and prevent the system from sleeping while `make` is running.

# Requirements
To run it, Windows 7 or later is required. To build it, a compiler that supports C++14 is required.

# Usage
```shell
$ caffeinate --help
Usage: caffeinate [OPTION...] [UTILITY...]
  prevent the system from sleeping

Options:
              --help  display this help and exit
  -d                  prevent the display from sleeping
  -i                  prevent the system from idle sleeping (default)
  -t TIMEOUT          specify the timeout value in seconds
  -w PID              wait for the process with the specified pid to exit
```

## Author
[@iorate](https://twitter.com/iorate)

## License
[Boost Software License 1.0](http://www.boost.org/LICENSE_1_0.txt)
