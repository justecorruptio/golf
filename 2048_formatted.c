M[99],k,j,l,P,x;
main(V){
    for(system("stty cbreak");
        k||(j=k=16,
            x=x^4
                ?   x==1
                    ?   read(V-=5,&P,3),P/7%6538
                    :   V&&M[V=rand()%16+1]
                        ?   2
                        :   (M[V]=2<<rand(puts("\e[H\e[J"))%2,V=4)
                :   1);
        l=P?P&~-~l:l,P=1)
        V|=(j&3|k==j
            ?   P=M[j--*x%17],
                x<2&&printf("%4.d|%c",P,j&3?:10,V*=P<2e3),
                P*l
            :   1
            )&&!(M[k--*x%17|68/x]=l+P&~P);
    puts(~V?"WIN":"LOSE");
}
