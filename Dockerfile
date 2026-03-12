FROM alpine

RUN apk add gcc make git linux-headers musl-dev

WORKDIR /app

COPY ./src .

RUN make

CMD ["./mcp"]