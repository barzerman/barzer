# converts output from dtaan barzer shell command  
# into a stmset 
BEGIN {
printf( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<stmset xmlns:xsi=\"http://www.w3.org/2000/10/XMLSchema-instance\"\nxmlns=\"http://www.barzer.net/barzel/0.1\">" );


}
/NAME/ {
	split( $0, f, "|" )
	id=f[1]
	split(f[2],eclass,":")
	id=substr(id,8)
	split( f[3], tok, " " )
	printf( "<stmt>" )
	printf( "<pat>" )
	for( t in tok ) {
		printf( "<t>%s</t>", tok[t] );
	}
	printf( "</pat><tran>" )
	printf ("<mkent i=\"%s\" c=\"%s\" s=\"%s\"/>", id, eclass[1], eclass[2] )
	printf( "</tran></stmt>\n" )
}
/FLUFF/ {
	printf ("***FLUFF:%s\n", $0 )
}
END {
printf( "</stmset>\n" );
}
