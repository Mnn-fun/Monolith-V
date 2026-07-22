-- ==============================================================================
-- MonolithV - Version 2 Oracle Database Schema Migration Script
-- ==============================================================================
-- Target Engine: Oracle Autonomous Transaction Processing (ATP) / 19c / 23ai
-- Purpose: Add defense-in-depth UNIQUE constraint to SHARE_EVENTS preventing
--          duplicate share transactions across concurrent attempts.
-- ==============================================================================

ALTER TABLE share_events
ADD CONSTRAINT uq_share_events_atomic UNIQUE (season_id, giver_player_id, receiver_player_id, item_type);
