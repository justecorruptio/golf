M[99],k,j,l,P,x;main(V){for(system("stty cbreak"
);k||(j=k=16,x=V<0?read(V/=2,&x,3),x/7%6538:V&&M
[P=rand()%k+1]?85:95|(M[P]=2<<rand(V=-9)%2));V-=
P&=2048)l^=k>j>j%4||printf("\e[H\e[J\n\0%4.d|%c"
+8/x,P=M[j*x%17],j%4*10)&&j--*P*l?V|=!(M[k--*x%
17|V&68]=l+P&~P),l|P:P;puts(~V?"WIN":"LOSE");}
