M[99],k,j,l,P,x;
main(V){
    for(system("stty cbreak");
        k||(j=k=16,
            x=V<0
                ?   read(V/=2,&x,3),x%52946/4
                :   V&&M[V=rand()%k+1]
                    ?   85
                    :   -(M[V]=2<<rand()%2,V=-4));
        V-=P&=2048)
        l^=k>j>j%4
        ||  printf(x^1?"\e[H\e[J\n":"%4.d|%c",P=M[j*x%17],j%4*10)
            &&j--*P*l
            ?   V|=!(M[k--*x%17|68/x]=l+P&~P),l|P
            :   P;
    puts(~V?"WIN":"LOSE");
}
