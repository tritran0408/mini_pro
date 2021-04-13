FROM gitpod/workspace-full-vnc
USER gitpod
RUN git clone https://github.com/akheron/jansson.git && \
    cd jansson && \
    autoreconf -fi  && \
    ./configure && \
    make all -j && \
    sudo make install && \
    cd .. && rm -rf jansson
    
RUN git clone https://github.com/benmcollins/libjwt.git  && \
    cd libjwt && \
    autoreconf --install && \
    ./configure && \
    make all -j && \
    sudo make install && cd .. && rm -rf libjwt
