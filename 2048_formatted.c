M[99],V=1,k,j,l,P;
s(x){
    V/=x;
    for(j=k=16;k;
        l=P?P&~-~l:l,P=1)
        V|=(j&3|k==j
            ?   P=M[j--*x%17],
                x<2&&printf("%4.d|%c",P,j&3?:10,V-=P&2048),
                P*l
            :   1
            )&&!(M[k--*x%17|68/x]=l+P&~P);
}
main(){
    for(system("stty cbreak");M[k=rand(puts("\e[H\e[J"))%16*V+V]||(M[k]=V<<rand()%2+1,s(4),s(1),read(V-1,&j,3)>0);)
        s(j/7%6538|4);
    puts(V?"WIN":"LOSE");
}
