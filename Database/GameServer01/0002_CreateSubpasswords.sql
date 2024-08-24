CREATE TABLE Subpasswords (
    AccountID INT NOT NULL,
    Type INT NOT NULL,
    Password VARCHAR(11) NOT NULL,
    Question INT UNSIGNED NOT NULL,
    Answer VARCHAR(128) NOT NULL,
    Locked BOOLEAN NOT NULL DEFAULT FALSE,
    RequestTimeout BIGINT UNSIGNED NOT NULL DEFAULT UNIX_TIMESTAMP(),
    FailureCount TINYINT NOT NULL DEFAULT 0,
    CreatedAt BIGINT UNSIGNED NOT NULL DEFAULT UNIX_TIMESTAMP(),
    UpdatedAt BIGINT UNSIGNED NOT NULL DEFAULT UNIX_TIMESTAMP(),

    PRIMARY KEY (AccountID),
    UNIQUE (AccountID, Type)
) ENGINE = INNODB DEFAULT CHARSET = utf8;