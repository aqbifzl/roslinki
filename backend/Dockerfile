FROM golang:1.24-alpine AS builder

RUN apk add --no-cache gcc musl-dev

WORKDIR /app

COPY go.mod go.sum ./
RUN go mod download

COPY . .

RUN CGO_ENABLED=1 GOOS=linux go build -o main .

FROM alpine:latest

WORKDIR /app

RUN apk add --no-cache ca-certificates

COPY --from=builder /app/main .

COPY index.html .

RUN mkdir -p /data

ENV HTTP_PORT=8080 \
    DB_PATH=/data/roslinki.db

EXPOSE 8080

CMD ["./main"]
