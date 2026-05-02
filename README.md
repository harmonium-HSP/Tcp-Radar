# TCP-Radar

TCP flow tracking and monitoring tool with congestion control visualization.

## Features

- Real-time TCP flow table management
- TCP state machine tracking
- Congestion control metrics (RTT, CWND, ssthresh)
- JSON serialization for WebSocket streaming
- Protocol parsing (Ethernet, IP, TCP headers)

## Building

```bash
make clean
make
```

## Testing

```bash
make test
```

## Project Structure

```
tcp-radar/
├── src/              # Source code
│   ├── flow.c        # Flow table implementation
│   ├── state_machine.c # TCP state machine
│   ├── congestion.c   # Congestion control
│   ├── parser.c      # Protocol parsing
│   └── json.c        # JSON serialization
├── include/          # Header files
├── tests/unit/      # Unit tests
└── Makefile
```

## License

MIT