#pragma once

struct NullEntry
{
	NullEntry() {}	//Default constructor needed for STL conatiners

	NullEntry(unsigned int entryPrimaryKey, unsigned int deviceID, unsigned int SDCounter, 
				unsigned int recordInsertedPrimaryKey, unsigned int requestCount):
					m_entryPrimaryKey{entryPrimaryKey},
					m_deviceID{deviceID},
					m_SDCounter{SDCounter},
					m_recordInsertedPrimaryKey{recordInsertedPrimaryKey},
					m_requestCount{requestCount}
	{ }

	//Useful when a NullEntry must be created without knowing m_entryPrimaryKey
	NullEntry(unsigned int deviceID, unsigned int SDCounter, unsigned int recordInsertedPrimaryKey, 
				unsigned int requestCount):
		m_deviceID{deviceID},
		m_SDCounter{SDCounter},
		m_recordInsertedPrimaryKey{recordInsertedPrimaryKey},
		m_requestCount{requestCount}
	{ }

	unsigned int m_entryPrimaryKey;
	unsigned int m_deviceID;
	unsigned int m_SDCounter;
	unsigned int m_recordInsertedPrimaryKey;
	unsigned int m_requestCount;
};
