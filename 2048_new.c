M[17],X=16,W,k;s(x,i,j,l,P,t){for(i=4;i--;x||puts
(""))for(j=k=l=0;k<4;l=j<4?W|=P=M[w(x,i,j++)],x||
printf(P?"%4d|":"    |",P),P?l?M[t]=l+P&~P,k++:0,
P&~l:l:(M[k++,t]=l,W&=~!l,0))t=x>1?w(x,i,k):X;}w(
d,i,j){return d?w(d-1,j,3-i):4*i+j;}main(i){for(i
=X+rand(k||system("stty cbreak"))%X;M[--i%X]*i;);
i?M[i%X]=2<<rand()%2:0;puts("\e[H\e[J"),s(W=1),s(
0);W&2049?puts(W>>11?"WIN":"LOSE"):read(0,&k,3)|
main(s(k%985));}
