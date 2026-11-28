CREATE TABLE `submission`
(
    `id` INTEGER PRIMARY KEY,
    `status` INTEGER NOT NULL DEFAULT 0,
    `time` TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `filename` VARCHAR(128) NOT NULL,
    `email` VARCHAR(128),
    `language` VARCHAR(5)
);
