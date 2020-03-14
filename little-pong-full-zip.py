from pygame import *
init();c=255;q=display.set_mode((2*c,c));x=50;y,u=c,c/2;w=Rect(25,x,5,x);e=w.move(2*c-x,0);i=o=1;g=h=0;s=lambda v:-1 if v<0 else 1;m=lambda v:min(c-x,max(v,0));a=lambda v:draw.rect(q,c,v)
while q.fill(time.wait(9)):
	event.get();k=key.get_pressed();k=3*(k[274]-k[273]);l=3*s(u-e.top);y+=i;u+=o;o*=s((c-u)*u);j=Rect(y,u,8,8);a(j);a(w);a(e);w.top=m(w.top+k);e.top=m(e.top+l);t=font.SysFont("",x).render(str(g)+" "+str(h),0,(x,x,c));q.blit(t,(c-g/10*20-17,9));display.flip();n=1+s(j.collidelist([w, e]));i=(i+s(i)*n)*(1-n);z=max(0,s(y-c));o+=s(k-z*k+l*z)*n
	if not 0<y<2*c:g+=z;h+=1-z;y,u=c,c/2;i=o=s(g-h)
quit()
