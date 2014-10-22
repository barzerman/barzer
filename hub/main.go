package main

import (
	//"bufio"
	"encoding/json"
	"encoding/xml"
	"flag"
	"fmt"
	"log"
	"net"
	"net/http"
	"os"
	"runtime"
	//"strings"
	"sort"
	"sync"
)

//flags
const CFG_DEFAULT = "/usr/share/barzer/config.xml"

var (
	cfgFile = flag.String("cfg", CFG_DEFAULT,
		fmt.Sprintf("Barzer Config File (default: %s)", CFG_DEFAULT))
	listenAddr = flag.String("listen", ":8090", "Listen on addr:port")
	verbose    = flag.Bool("v", false, "Verbose output")
)

// types

type HostRec struct {
	uid    uint32
	addr   string
	weight float64
}

type Settings struct {
	hosts     []*HostRec
	weightSum float64
}

type EntList []*BeniEnt

func (l EntList) Len() int           { return len(l) }
func (l EntList) Swap(i, j int)      { l[i], l[j] = l[j], l[i] }
func (l EntList) Less(i, j int) bool { return l[i].Score <= l[j].Score }

var settings = Settings{weightSum: 0}
var END_TOKEN = []byte("\r\n.\r\n")

// ----l-------------------
func query(w http.ResponseWriter, req *http.Request) {
	query := BarzerQuery{Beni: "b", Ret: "json"}
	req.ParseForm()
	query.Data = req.FormValue("query")
	if query.Data == "" {
		WriteError(w, "No query set")
		return
	}
	hosts := settings.hosts
	size := len(hosts)
	results := make([]*Barz, size, size)
	var wg sync.WaitGroup

	queryBarzer := func(i int, host *HostRec, q BarzerQuery) {
		defer wg.Done()
		conn, err := net.Dial("tcp", host.addr)
		if err != nil {
			log.Println("Error connecting to barzer server at ", host.addr, err)
			return
		}
		defer conn.Close()
		q.Uid = host.uid

		data, err := xml.Marshal(q)
		if err != nil {
			log.Println("Error marshalling barzer query:", err)
			return
		}

		conn.Write(data)
		conn.Write(END_TOKEN)
		if *verbose {
			log.Print(string(data))
		}

		var barz Barz
		dec := json.NewDecoder(conn)
		if err = dec.Decode(&barz); err != nil {
			log.Println("Error decoding barzer response: ", err)
			return
		}
		barz.Weight = host.weight
		results[i] = &barz
	}
	wg.Add(size)
	for i, host := range hosts {
		go queryBarzer(i, host, query)
	}
	wg.Wait()

	cache := map[string]*BeniEnt{}
	for _, barz := range results {
		if barz == nil {
			continue
		}
		for _, ent := range barz.Beni {
			stored, ok := cache[ent.Id]
			if !ok {
				stored = &ent
				cache[ent.Id] = stored
			}
			stored.Score += ent.Cover * barz.Weight
		}
	}

	final := make(EntList, len(cache))
	i := 0
	for _, ent := range cache {
		ent.Score /= settings.weightSum
		final[i] = ent
		i++
	}

	sort.Sort(sort.Reverse(EntList(final)))
	result := JSONResult{BarzList: results, Final: final}

	enc := json.NewEncoder(w)
	if err := enc.Encode(&result); err != nil {
		log.Println("Error encoding result array: ", err)
	}
}

// loads a user@host:port per line file
func loadConfig(settings *Settings) {
	fh, err := os.Open(*cfgFile)
	if err != nil {
		log.Println("Unable to open", *cfgFile)
		return
	}
	defer fh.Close()
	log.Println("Loading hosts from", *cfgFile)

	var cfg BarzerConfig
	dec := xml.NewDecoder(fh)
	if err = dec.Decode(&cfg); err != nil {
		log.Println("Error decoding barzer config: ", err)
		return
	}

	for _, inst := range cfg.Instances {
		for _, uni := range cfg.Universes {
			hostRec := &HostRec{
				uid:    uni.Uid,
				addr:   fmt.Sprintf("%s:%d", inst.Host, inst.Port),
				weight: inst.Weight + uni.Weight,
			}
			if *verbose {
				log.Printf("Adding host: (%d@%s = %.0f)",
					hostRec.uid, hostRec.addr, hostRec.weight)
			}
			settings.hosts = append(settings.hosts, hostRec)
			settings.weightSum += hostRec.weight
		}
	}
}

func main() {
	runtime.GOMAXPROCS(runtime.NumCPU())
	flag.Parse()
	loadConfig(&settings)
	http.HandleFunc("/query", query)
	log.Printf("Serving at %s...\n", *listenAddr)
	err := http.ListenAndServe(*listenAddr, nil)
	if err != nil {
		log.Fatal("Unable to listen: ", err)
	}
}
