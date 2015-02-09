REGISTER(){
	EXEC ${CASCADE_BIN}resample $2 $1 $3 $4 $5
}
REGISTER_VECTOR(){
	EXEC ${CASCADE_BIN}resampleVector $2 $1 $3 $4 $5
}