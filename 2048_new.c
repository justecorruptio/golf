M[16],X=16,W,k;s(f,d,i,j,l,P,t){for(i=4;i--;)
for(j=k=l=0;k<4;)t=w(d,i,k),j<4?P=M[w(d,i,j++
)],W|=P>>11,l*P?f?M[t]=l<<(l==P):0,k++:0,l=P?
l-P?P:0:l:(f?M[t]=l:0,++k,W|=2*!l,l=0);}w(d,i
,j){return d?w(d-1,j,3-i):4*i+j;}main(i){k||
system("stty cbreak");for(i=X+rand()%X;M[i%X]
*i;i--);i?M[i%X]=2<<rand()%2:0;s(W=0,0),s(0,1
);for(i=X,puts("\e[H\e[J");i--;i%4||puts(""))
printf(M[i]?"%4d|":"    |",M[i]);W-2?puts(W&1
?"WIN":"LOSE"):read(0,&k,3)|main(s(1,k%7));}
