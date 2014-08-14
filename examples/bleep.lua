function boo(d1, d2, p)
     function pause() lblink.sleep(p) end
     d1:red();    pause();
     d2:blue();   pause();
     d1:green();  pause();
     d2:yellow(); pause();
     d1:orange(); pause();
     d1:off();    d2:off();
 end
 
