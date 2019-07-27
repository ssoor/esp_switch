package main

import (
	"fmt"
	"io"
	"net"
)

func connIO(conn net.Conn) {
	status := false
	buff := make([]byte, 0, 1)

	for {
		if _, err := io.ReadFull(conn, buff[:1]); nil != err {
			fmt.Println("readfull failed, error:", err)
			return
		}

		status = !status
		buff[0] = 0
		if status {
			buff[0] = 1
		}

		conn.Write(buff[:1])
	}
}

func main() {
	l, err := net.Listen("tcp", ":65530")
	if nil != err {
		fmt.Println("listen failed, error:", err)
		return
	}

	for {
		conn, err := l.Accept()
		if nil != err {
			fmt.Println("accept failed, error:", err)
			return
		}

		go connIO(conn)
	}

}
