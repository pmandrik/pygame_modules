from pygame import *
init();c=250;screen=display.set_mode((600,480));z=[Rect(55+x%10*50,100+y/10*15,40,12) for x,y in enumerate(xrange(50))];y=u=c;i=o=-3;p=Rect(c,460,100,10);s=lambda v:-1 if v<0 else 1;m=lambda v,q,w:min(w,max(v,q))
while u<480:
	screen.fill(time.wait(9));event.get();k=key.get_pressed();k=k[276]-k[275];draw.rect(screen,c,p);p.left=m(p.left-8*k,0,2*c);draw.circle(screen,c,(y,u),5);y+=i;u+=o;i*=s((600-y)*y);o*=s(u)
	for r in z: 
		if draw.rect(screen,c,r) and r.collidepoint(y,u):d=min(r.bottom-u,u-r.top)-min(r.right-y,y-r.left);o*=s(d);i*=-s(d);z.remove(r)
	if p.collidepoint(y,u) or display.flip():o,i=-o,m(i-k,-5,5)
quit()
