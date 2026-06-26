M[17],X=16,W,k;s(x,i,j,l,P,t){for(i=4;i--;)
for(j=k=l=0;k<4;j<4?P=M[w(x%7,i,j++)],W|=P>>
11,l*P?M[t]=l<<(l==P),k++:0,l=P?l-P?P:0:l:(M[
t]=l,++k,W|=2*!l,l=0))t=x>X?w(x%7,i,k):X;}w(d
,i,j){return d?w(d-1,j,3-i):4*i+j;}main(i){
for(i=X+rand(k||system("stty cbreak"))%X;M[i%
X]*i;i--);i?M[i%X]=2<<rand()%2:0;W=0;for(i=X,
puts("\e[H\e[J");i--;s(i),i%4||puts(""))
printf(M[i]?"%4d|":"    |",M[i]);W-2?puts(W&1
?"WIN":"LOSE"):read(0,&k,3)|main(s(k));}
