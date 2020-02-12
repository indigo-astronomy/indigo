FROM arm32v7/debian:buster-slim

RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y \
      sudo gnupg udev wget iproute2 runit-init systemd iw fxload hostapd dnsmasq dhcpcd5 && \
    wget -O - https://www.astroberry.io/repo/key | sudo apt-key add - && \
    sudo su -c "echo 'deb https://www.astroberry.io/repo/ buster main' > /etc/apt/sources.list.d/astroberry.list" && \
    sudo su -c "echo 'deb [trusted=yes] https://indigo-astronomy.github.io/indigo_ppa/ppa indigo main' > /etc/apt/sources.list.d/indigo.list" && \
    sudo apt-get update && \
    sudo apt-get upgrade -y && \
    sudo apt-get install -y indigo && \
    apt-get -y --purge autoremove && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY tools/shutdown /sbin/shutdown

COPY run.sh /app/run.sh
ENTRYPOINT ["/app/run.sh"]
