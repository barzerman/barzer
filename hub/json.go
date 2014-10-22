package main

import (
	"encoding/json"
	"io"
	"log"
)

type BeniEnt struct {
	Rank     uint32  `json:"rank"`
	Class    uint32  `json:"class"`
	Subclass uint32  `json:"subclass"`
	Cover    float64 `json:"cover"`
	Rel      float64 `json:"rel"`
	Type     string  `json:"type"`
	Id       string  `json:"id"`
	Name     string  `json:"name"`
	Scope    string  `json:"scope"`
	Category string  `json:"category"`
	Score    float64 `json:"rscore"`
}

type Barz struct {
	Beni   []BeniEnt `json:"beni"`
	Uid    uint32    `json:"uid"`
	Weight float64   `json:-`
}

type JSONError struct {
	Error string `json:"error"`
}

type JSONResult struct {
	BarzList []*Barz    `json:"barzlist"`
	Final    []*BeniEnt `json:"final"`
}

func WriteError(w io.Writer, estr string) {
	jerr := JSONError{Error: estr}
	data, err := json.Marshal(&jerr)
	if err != nil {
		log.Printf("Error marshalling %s: %s\n", estr, err)
		return
	}
	w.Write(data)
}
