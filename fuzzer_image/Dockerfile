FROM alpine

RUN apk update

RUN apk add ncurses-dev gcc libc-dev flex bison linux-headers make git

COPY debugrc /root/.notdnethackrc

WORKDIR /app

RUN git clone --depth 1 https://github.com/demogorgon22/notdnethack.git --branch master dnh

RUN cd dnh && sed -i -e "s/\/\* set nvrange \*\//iflags.debug_fuzzer = 1;/" src/u_init.c
#RUN cd dnh && sed -i -e "s/\/\* set nvrange \*\//*(int *)u.ux=0;/" src/u_init.c

RUN cd dnh && make -j install

#RUN mv /app/dnh/dnethackdir /

#RUN rm -rf /app

#WORKDIR /dnethackdir/

RUN mkdir /tmp/cores

COPY init.sh /

CMD /init.sh
