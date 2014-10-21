package main

type EntityKey struct {
	Class    uint32
	Subclass uint32
	Id       string
}

type EntityScore struct {
	Cover  float64
	Weight float64
}

type EntityRec struct {
	Ent    BeniEnt
	Scores []EntityScore
}

type EntityMap map[EntityKey]EntityRec
