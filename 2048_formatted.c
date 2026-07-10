M[99],W,k,G,i,j,l,P,B;
s(x){
    G=5-x%11;
    for(i=4;i--;)
        for(B=4/G*i^x%7*G,j=k=0;k<4;
            W|=l=P?P&~-~l:l)
            if(j<4
            ?   P=M[B^G*j++],
                x&64&&printf("%4.d|%c",P,6%j*5),
                P*l
            :   (W&=~!l,P=1)
            )M[B^G*k+++x-x%2*x]=l+P&~P;
}
main(){
    for(;~W%2&M[i=rand(puts("\e[H\e[J"))%16+W%2*16];);
    M[i]=2<<rand(W=k||system("stty cbreak"))%2,
    s(56),s(70),
    W&2049?puts(W>>11?"WIN":"LOSE"):read(0,&k,W=3)|main(s(k%162));
}
