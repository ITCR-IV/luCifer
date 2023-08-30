# syntax=docker/dockerfile:1

FROM alpine:3.14
RUN apk add --no-cache bash
RUN apk add --no-cache curl
COPY client.sh /client.sh
RUN chmod +x /client.sh
ENTRYPOINT ["/client.sh"]
