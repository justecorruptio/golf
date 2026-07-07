M[99],W,k,G,i,j,l,P,t,B;

s(x){
    G=5-x%11;
    for(i=4;i--;)
        for(j=k=0;k<4;
            l=j<4
            ?   W|=P=M[B^G*j++],
                x&64&&printf("%4.0d|%c",P,j/4*10),
                P?l?M[k++,t]=l+P&~P:0,P&~l:l
            :   (M[k++,t]=l,W&=~!l,0))
        B=x%11*i^x%7*G,t=x&1?B^G*k:x;
}

main(){
    W%2|!M[i=rand(puts("\e[H\e[J"))%16+W%2*16]
    ?   M[i]=2<<rand(W=k||system("stty cbreak"))%2,
        s(56),s(70),
        W&2049?puts(W>>11?"WIN":"LOSE"):read(0,&k,W=3)|main(s(k%162))
    :   main();
}
