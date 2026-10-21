M[99],V,k,G,i,j,l,P,B;
s(x){
    V/=G=x%5;
    for(i=4;i--;)
        for(B=4/G*i^x%9*G,j=k=4;k;
            l=P?P&~-~l:l)
            if(j
            ?   P=M[B^G*--j],
                x&2&&printf("%4.d|%c",P,!j*10,V-=P&2048),
                P*l
            :   (V|=!l,P=1)
            )x%2?M[B^G*--k]=l+P&~P:--k;
}
main(){
    for(s(k%664*3?:!system("stty cbreak"));M[i=rand(puts("\e[H\e[J"))%16]*V;);
    M[i]+=V+V<<rand()%2,
    s(84),s(66),
    V>0?main(read(V=0,&k,3)):puts(V?"WIN":"LOSE");
}
