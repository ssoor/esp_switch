package main

import (
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"net"
	"net/http"
	"os"
)

var (
	server serve
)

type serve struct {
}

func (s serve) ServeHTTP(resp http.ResponseWriter, req *http.Request) {
	var conf conf

	switch req.FormValue("switch") {
	case "0":
		conf.Switch = true
	case "1":
		conf.Switch = false
	default:
		jsonBin, err := ioutil.ReadFile("conf.json")
		if nil != err {
			resp.WriteHeader(http.StatusInternalServerError)
			resp.Write([]byte(fmt.Errorf("ReadFile failed, error: %v", err).Error()))
			return
		}

		if err := json.Unmarshal(jsonBin, &conf); nil != err {
			resp.WriteHeader(http.StatusInternalServerError)
			resp.Write([]byte(fmt.Errorf("Unmarshal failed, error: %v", err).Error()))
			return
		}
	}

	jsonBin, err := json.Marshal(&conf)
	if nil != err {
		resp.WriteHeader(http.StatusInternalServerError)
		resp.Write([]byte(fmt.Errorf("Marshal failed, error: %v", err).Error()))
		return
	}

	if err := ioutil.WriteFile("conf.json", jsonBin, 0655); nil != err {
		resp.WriteHeader(http.StatusInternalServerError)
		resp.Write([]byte(fmt.Errorf("WriteFile failed, error: %v", err).Error()))
		return
	}

	switch req.URL.Path {
	case "/switch":
		resp.Write(jsonBin)
		resp.Header().Add("Content-Type", "application/json")
		return
	}

	htmlBin, err := ioutil.ReadFile("switch.html")
	if nil != err {
		resp.WriteHeader(http.StatusInternalServerError)
		resp.Write([]byte(fmt.Errorf("ReadFile failed, error: %v", err).Error()))
		return
	}

	resp.Write(htmlBin)
}

type conf struct {
	Switch bool `json:"switch"`
}

func connIO(conn net.Conn) {
	var conf conf
	buff := make([]byte, 1)

	for {
		if _, err := io.ReadFull(conn, buff[:1]); nil != err {
			fmt.Println("ReadFull failed, error:", err)
			return
		}

		jsonBin, err := ioutil.ReadFile("conf.json")
		if nil != err {
			fmt.Println("ReadFile failed, error:", err)
			continue
		}

		if err := json.Unmarshal(jsonBin, &conf); nil != err {
			fmt.Println("Unmarshal failed, error:", err)
			continue
		}

		buff[0] = 1
		if conf.Switch {
			buff[0] = 0
		}

		fmt.Println("change switch status to", buff[0])
		conn.Write(buff[:1])
	}
}
func main() {
	l, err := net.Listen("tcp", "0.0.0.0:65530")
	if nil != err {
		fmt.Println("listen failed, error:", err)
		return
	}

	go func() {
		if err := http.ListenAndServe(":65531", &server); nil != err {
			fmt.Println("http server start failed, error:", err)
			os.Exit(-1)
		}
	}()

	for {
		conn, err := l.Accept()
		if nil != err {
			fmt.Println("accept failed, error:", err)
			return
		}

		fmt.Println("new client, remote addr:", conn.RemoteAddr())

		go connIO(conn)
	}

}
