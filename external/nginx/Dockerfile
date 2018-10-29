FROM base/archlinux

ENV IFACE=p1p1
RUN pacman -Syu --noconfirm && pacman -S --noconfirm geoip git dnsmasq
ADD run_nginx.sh nginx/objs/nginx /bin/
RUN chmod +x /bin/run_nginx.sh
ADD qkdsecure.* nginx/conf/* /conf/
ADD softhsm2.conf /etc
ADD libsofthsm2.so /usr/lib
ADD dnsmasq.conf /etc/
RUN git clone https://github.com/jpetazzo/pipework.git && \
    mkdir -p /logs && \
    useradd -M -r -s /bin/false -U nginx && \
    chown nginx /conf/qkdsecure.key && \
    chown -R nginx /logs
# The softhsm token folder is owned by the user so running as nginx user means it cant read the tokens
#USER nginx

ADD html /

ENTRYPOINT /bin/run_nginx.sh $IFACE
LABEL description="docker run -it --rm -v /var/lib/softhsm:/var/lib/softhsm --env IFACE=p1p1 qkdsecure"
EXPOSE 80 443
