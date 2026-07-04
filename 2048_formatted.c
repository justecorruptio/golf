M[17],X=16,W,k,I,G,m;

s(x,i,j,l,P,t){
    I=4-x%2*3;
    G=5-I;
    m=x&2?3:0;
    for(i=4;i--;)
        for(j=k=l=0;k<4;
            l=j<4
            ?   W|=P=M[i*I+G*(j++^m)],
                x||printf("%4.0d|%c",P,j/4*10),
                P?l?M[k++,t]=l+P&~P:0,P&~l:l
            :   (M[k++,t]=l,W&=~!l,0))
        t=x>1?i*I+G*(k^m):X;
}

main(i){
    for(i=X+rand(k||system("stty cbreak"))%X;M[--i%X]*i;);

    M[i?i%X:X]=2<<rand(puts("\e[H\e[J"))%2;

    s(W=1),s(0);

    W&2049?puts(W>>11?"WIN":"LOSE"):read(0,&k,3)|main(s(k%985));
}
