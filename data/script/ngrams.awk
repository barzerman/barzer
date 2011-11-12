BEGIN {
    print "reading names.txt, analyzing n-grams"
    numUgram=0
    numBigram=0
    numTrigram=0
    while ((getline line < "names.txt") > 0) {
        split(line,arr)
        for( i in arr ) {
            ugram[arr[i]] += 1
            if( arr[i+1] ) {
                bg=arr[i]" "arr[i+1] 
                bigram[ bg ] += 1
                
                if( arr[i+2] ) {
                    tg=bg" "arr[i+2]
                    trigram[ tg ] += 1
                }
            }
        }
    }
    for( i in ugram ) { ++numUgram }
    for( i in bigram ) { ++numBigram }
    for( i in trigram ) { ++numTrigram }

    print "Ugrams:",numUgram, "Bigrams:",numBigram, "trigrams:", numTrigram
    print "reading names.txt, analyzing 2-grams"
    minNum=10 
    for( j in ugram ) {
        if( ugram[j] > minNum )
            print "UGRAM|",ugram[j],j
    }
    minBigram=minNum*(numBigram/numUgram)
    for( j in bigram ) {
        if( bigram[j] > minBigram )
            print "BIGRAM|",bigram[j],j
    }
    minTrigram=minNum*(numTrigram/numUgram)
    for( j in trigram ) {
        if( trigram[j] > minTrigram )
            print "TRIGRAM|",trigram[j],j
    }
    exit
}
{
}
