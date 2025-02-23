# Switching - Lab 1. Route Lookup
**`[GITT]` `[Sem 3.2]` `[Swit]` `(Spring 2025)`**

**Telecommunication Technologies Engineering** | *Universidad Carlos III de Madrid*

---

## Project Overview

This project implements an IP route lookup algorithm in C to determine the
output interface for forwarding packets based on a Forwarding Information Base
(FIB).

* **Algorithm**: LC-tries
* **Description**: Implements route lookups using LC-tries, optimizing Patricia
  tries for speed and memory.

### Lab group - Authors

| Name                            | Student ID |
| ------------------------------- | ---------- |
| Alonso Herreros Copete          | 100493990  |
| Ismael Martín de Vidales Martín | 100496143  |

## Compilation

Use the provided `Makefile` to compile.

1. Navigate to the project directory.
2. Run `make all` or `make my_route_lookup`.

To clean build files, use `make clean`.

## Usage

Run the program with:

```sh
./my_route_lookup FIB InputPacketFile
```

* `FIB`: Path to the FIB file.
* `InputPacketFile`: Path to the input packet file.

### Input File Format

* **FIB**: `<CIDR_Network_Prefix>\t<Output Interface>` per line.
* **Input Packet**: One destination IPv4 address per line.

### Output

The program creates `InputPacketFile.out` with results in the format:
`<IPaddress>;<OutIfc>;<AccessedNodes>;<ComputationTime>`. The file includes
summary statistics such as the **total number of processed destination IP
addresses**, the **average number of accessed nodes by lookup**, and the
**average calculation time** for the processed addresses. The output file will
also include the number of nodes in the tree, packets processed, average node
accesses, average packet processing time, memory usage, and CPU time.

## References and Resources

* M. Waldvogel, G. Varghese, J. Turner, and B. Plattner. Scalable High Speed
  Prefix Matching. ACM Transactions on Computer Systems, 2001
* Route lookup en Linux -
  <https://vincent.bernat.ch/en/blog/2017-ipv4-route-lookup-linux>
* “IP-address lookup using LC-tries,” IEEE Journal on Selected Areas in
  Communications, 17(6):1083-1092, June 1999.
  <https://dl.acm.org/doi/10.1109/49.772439>
* [VirtualBox OVA](https://aulaglobal.uc3m.es/mod/url/view.php?id=5336886)
