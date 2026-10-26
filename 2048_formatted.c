M[99],k,j,l,P,x;
main(V){
    for(system("stty cbreak");
        k||(j=k=16,
            x=x^6
                ?   x&2
                    ?   read(V-=7,&P,3),P/7&8125
                    :   V&&M[V=rand()%k+1]
                        ?   4
                        :   (M[V]=2<<rand()%2,V=6)
                :   puts("\e[H\e[J"));
        l=P?P&~-~l:l,P=1)
        V|=(j&3|k==j
            ?   P=M[j--*x%17],
                x%7||printf("%4.d|%c",P,j&3?:10,V*=P<2e3),
                P*l
            :   1
            )&&!(M[k--*x%17|119/x]=l+P&~P);
    puts(~V?"WIN":"LOSE");
}
