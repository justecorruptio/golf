M[99],W,k,G,i,j,l,P,B;
s(x){
    G=5-x%5;
    for(i=4;i--;)
        for(B=4/G*i^x%7*G,j=k=4;k;
            W|=l=P?P&~-~l:l)
            if(j
            ?   P=M[B^G*--j],
                x&2&&printf("%4.d|%c",P,!j*10),
                P*l
            :   (W&=~!l,P=1)
            )x%2?M[B^G*--k]=l+P&~P:--k;
}
main(){
    for(;~W%2&M[i=rand(puts("\e[H\e[J"))%16+W%2*16];);
    M[i]=2<<rand(W=G||system("stty cbreak"))%2,
    s(56),s(94),
    W&2049?puts(W>>11?"WIN":"LOSE"):read(0,&k,W=3)|main(s(k*3%42972));
}
