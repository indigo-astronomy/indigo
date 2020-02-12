FROM ubuntu:bionic

RUN apt-get update && \
    apt-get install -y \
      sudo udev software-properties-common \
      fxload && \
    sudo apt-add-repository -y ppa:mutlaqja/ppa && \
    sudo su -c "echo 'deb [trusted=yes] https://indigo-astronomy.github.io/indigo_ppa/ppa indigo main' > /etc/apt/sources.list.d/indigo.list" && \
    sudo apt-get update && \
    sudo apt-get upgrade -y && \
    sudo apt-get install -y indigo && \
    apt-get -y --purge autoremove && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY run.sh /app/run.sh
ENTRYPOINT ["/app/run.sh"]
