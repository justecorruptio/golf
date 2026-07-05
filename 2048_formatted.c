M[17],X=16,W,k,I,G,i,j,l,P,t;

s(x){
    I=x%11;
    G=5-I;
    for(i=4;i--;)
        for(j=k=0;k<4;
            l=j<4
            ?   W|=P=M[i*I+G*(j++^x%7)],
                x&64&&printf("%4.0d|%c",P,j/4*10),
                P?l?M[k++,t]=l+P&~P:0,P&~l:l
            :   (M[k++,t]=l,W&=~!l,0))
        t=x&1?i*I+G*(k^x%7):X;
}

main(){
    for(i=X+rand(k||system("stty cbreak"))%X;M[--i%X]*i;);

    M[i?i%X:X]=2<<rand(puts("\e[H\e[J"))%2;

    W=1,s(56),s(70);

    W&2049?puts(W>>11?"WIN":"LOSE"):read(0,&k,3)|main(s(k%162));
}
